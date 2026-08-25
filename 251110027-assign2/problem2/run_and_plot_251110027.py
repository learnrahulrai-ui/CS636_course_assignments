import subprocess
import re
import matplotlib.pyplot as plt


MY_QUEUE_EXE = "./my_queue"
BOOST_QUEUE_EXE = "./boost_queue"

# The parameters to test
threads = [1, 2, 4, 8, 16]
operations = [100000, 1000000, 10000000]

# Dictionaries to store the results
custom_data = {op: [] for op in operations}
boost_data = {op: [] for op in operations}

custom_regex = re.compile(r"\[MY QUEUE\] Time Taken:\s+([0-9.]+)\s*ms")
boost_regex = re.compile(r"\[BOOST\] Time Taken:\s+([0-9.]+)\s*ms")

def run_benchmark(executable, thread_count, op_count, regex_pattern):
    cmd = [executable, str(thread_count), str(op_count)]
    print(f"Running: {' '.join(cmd)} ... ", end="", flush=True)
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        
        match = regex_pattern.search(result.stdout)
        if match:
            time_ms = float(match.group(1))
            print(f"{time_ms} ms")
            return time_ms
        else:
            print("FAILED TO PARSE OUTPUT")
            print(f"Raw Output:\n{result.stdout}")
            return 0.0
            
    except subprocess.CalledProcessError as e:
        print(f"PROGRAM CRASHED with code {e.returncode}")
        return 0.0

if __name__ == "__main__":

    # Check if the executables are present
    for exe in [MY_QUEUE_EXE, BOOST_QUEUE_EXE]:
        if not subprocess.run(["which", exe], capture_output=True).stdout.strip():
            print(f"ERROR: Executable '{exe}' not found. Please compile it first.")
            exit(1)

    print("========================================")
    print("   AUTOMATED BENCHMARK SUITE STARTED    ")
    print("========================================")

    for op in operations:
        print(f"\n--- Testing with {op} Operations ---")
        for t in threads:
            c_time = run_benchmark(MY_QUEUE_EXE, t, op, custom_regex)
            custom_data[op].append(c_time)
            
            b_time = run_benchmark(BOOST_QUEUE_EXE, t, op, boost_regex)
            boost_data[op].append(b_time)

    print("\nAll benchmarks complete! Generating plot...")


    plt.figure(figsize=(10, 7))


    colors = {100000: 'blue', 1000000: 'green', 10000000: 'red'}
    labels = {100000: '1e5 ops', 1000000: '1e6 ops', 10000000: '1e7 ops'}

    for op in operations:
        color = colors[op]
        label = labels[op]
        
        plt.plot(threads, custom_data[op], marker='o', color=color, linestyle='-', 
                label=f'My Queue ({label})')
        
        plt.plot(threads, boost_data[op], marker='s', color=color, linestyle='--', 
                label=f'Boost Queue ({label})')


    plt.title('Lock-Free Queue Scalability: My Queue vs Boost Queue')
    plt.xlabel('Number of Threads')
    plt.ylabel('Time Taken (ms) - Logarithmic Scale')

    plt.yscale('log') 
    plt.xticks(threads) 

    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left') 

    plt.tight_layout()
    plt.savefig('scalability_plot.png', dpi=300)
    print("Plot saved as 'scalability_plot.png'. Opening window...")
    plt.show()