#include<bits/stdc++.h>

using namespace std;

using TID = int;

int NUM_THREADS = -1;
int NEXT_CHILD_TID = 1;

int parent_waiting_for_join = 0;
queue<TID> completed_threads;


struct VectorClock {

    vector<int> clock;

    void initializeVC(int tid = -1) {
        assert(NUM_THREADS != -1);
        clock.resize(NUM_THREADS, 0);
        if(tid != -1)
            clock[tid] = 1;
    }

    // Returns the size of the vector clock
    int returnSize(){
        return clock.size();
    }

    // Increment the clock for a thread
    void tick(int tid) {
        assert(tid < NUM_THREADS && tid < clock.size());
        clock[tid]++;
    }
};

// Structure to hold vector clock for each thread
struct ThreadState {
    VectorClock VC;

    ThreadState(int tid){
        VC.initializeVC(tid);
    }
};

// Structure to hold vector clock for each locks
struct LockState {
    VectorClock VC;
    
    LockState(){
        VC.initializeVC();
    }
};

// For every memory access to shared variable, we track the history of Reads and Writes.
struct VariableState {
    VectorClock WriteVC;
    VectorClock ReadVC;
    

    string lastWriteIP = "Nothing";
    TID lastWriteTID;
    
    string lastReadIP = "Nothing";
    TID lastReadTID;

    VariableState(){
        WriteVC.initializeVC();
        ReadVC.initializeVC();
    }
};

// Helper function to check partial order relation between two vector clocks
bool happensBeforeCheck(VectorClock a, VectorClock b){
    bool happensBefore = true;
    
    // CHECK: If the vector clocks are of the same size
    assert(a.returnSize() == b.returnSize());

    int clockSize = a.returnSize();
    for(int i=0;i<clockSize;i++){
        happensBefore &= (a.clock[i] <= b.clock[i]);
    }
    
    return happensBefore;
}

// Helper function to perform join of vector clocks
VectorClock joinVectorClocks(VectorClock a, VectorClock b){
    VectorClock c;
    // CHECK: If both clocks have the same size
    assert(a.returnSize() == b.returnSize());
    int clockSize = a.returnSize();
    c.initializeVC();
    for(int i=0;i<clockSize;i++){
        c.clock[i] = max(a.clock[i], b.clock[i]); 
    }
    return c;
}

// ============
// GLOBAL STATE
// ============

unordered_map<TID, ThreadState*> Threads;
unordered_map<string, LockState*> Locks;
unordered_map<string, VariableState*> Memory;
unordered_map<string, int> ipRaceCounts;

// Helper to ensure thread exists in our map
ThreadState* getThread(TID tid) {
    assert(tid < NUM_THREADS && tid < Threads.size());
    if (Threads.find(tid) == Threads.end()) {
        cerr << "Error: Thread " << tid << " not found!" << endl;
        exit(1);
    }
    return Threads[tid];
}

// Helper function to populate the race report
void updateRaceReport(string ip1, string ip2, string addr, string type, int tid1, int tid2){
    int newtid1 = (tid1 < tid2) ? tid1 : tid2;
    int newtid2 = (tid1 < tid2) ? tid2 : tid1;
    string ip_key = ip1 + "-" + ip2;
    string key = ip_key + " " + addr + " " + type + " " + to_string(newtid1) + "-" + to_string(newtid2);
    ipRaceCounts[key]++;
}


void printRaceReports(){
    if(!ipRaceCounts.empty()){
        int count = 0;
        for(auto& entry : ipRaceCounts){
            count += entry.second;
        }
        cout << count << "\n";
        for(auto& entry : ipRaceCounts){
            cout << entry.first << " " << entry.second << "\n";
        }
    }
}

// ===============
// EVENT HANDLERS
// ===============

void handleThreadCreate(TID parentTid) {
    TID childTid = NEXT_CHILD_TID++;
    // CHECK: The parent thread should be the main thread (TID 0).
    assert(parentTid == 0);
    // CHECK: We should not exceed the maximum number of threads.
    assert(childTid < NUM_THREADS);
    // Initialize with parent vector clock
    Threads[childTid] = new ThreadState(childTid);
    // join the vector clocks of parent (TID 0) and child
    Threads[childTid]->VC = joinVectorClocks(Threads[parentTid]->VC, Threads[childTid]->VC);
    // Increment the vector clock for synchronized begin operation
    Threads[parentTid]->VC.tick(parentTid);
}

void handleThreadJoin(TID parentTid) {
    // Check if there is a completed thread we can join with immediately
    if (!completed_threads.empty()) {
        TID childTid = completed_threads.front();
        completed_threads.pop();

        assert(parentTid == 0); // Only main thread should be joining
        
        // Perform JOIN (Parent.VC = max(Parent.VC, Child.VC))
        Threads[parentTid]->VC = joinVectorClocks(Threads[parentTid]->VC, Threads[childTid]->VC);
        
        // Increment Child VC
        Threads[childTid]->VC.tick(childTid);
    } else {
        // No child has ended yet, Parent must wait
        parent_waiting_for_join++;
    }
}

void handleChildEnd(TID childTid) {
    // Check if Parent is already waiting at pthread_join
    if (parent_waiting_for_join > 0) {
        // Perform JOIN immediately
        Threads[0]->VC = joinVectorClocks(Threads[0]->VC, Threads[childTid]->VC);
        Threads[childTid]->VC.tick(childTid);
        
        // Reset flag
        parent_waiting_for_join--;
    } else {
        // Parent hasn't called join yet, store this child as completed
        completed_threads.push(childTid);
    }
}

void handleChildBegin(TID childTid) {
    // Verify that the child thread state has been initialized by the parent
    assert(Threads.find(childTid) != Threads.end());
}

void handleLockAcquire(TID tid, string lockAddr) {
    // Initialize for a new lock
    if(Locks.find(lockAddr) == Locks.end()){
        Locks[lockAddr] = new LockState();
    }
    // Join the vector clocks of thread and lock
    Threads[tid]->VC = joinVectorClocks(Threads[tid]->VC, Locks[lockAddr]->VC);
}

void handleLockRelease(TID tid, string lockAddr) {
    // CHECK: The lock variable is acquired previously by any thread
    assert(Locks.find(lockAddr) != Locks.end());
    // Update the lock vector clock with the current thread vector clock
    Locks[lockAddr]->VC = Threads[tid]->VC;
    // Increment the vector clock for synchronized release operation
    Threads[tid]->VC.tick(tid);
}

void handleMemoryAccess(TID tid, string ip, string addr, bool isRead) {

    // Initialize for a new variable
    if(Memory.find(addr) == Memory.end()){
        Memory[addr] = new VariableState();
    }

    ThreadState* tState = getThread(tid);
    VariableState* mState = Memory[addr];
    
    if (isRead) {
        // DJIT+ Read Check

        bool nowrRace = true;
        if(mState->lastWriteTID != tid){
            nowrRace = happensBeforeCheck(mState->WriteVC, tState->VC);
        }

        // check for (Write-Read) race
        if(!nowrRace){
            updateRaceReport(mState->lastWriteIP, ip, addr, "W-R", mState->lastWriteTID, tid);
        }
        mState->lastReadTID = tid;
        mState->lastReadIP = ip;
        // Update the read vector clock of the shared variable
        mState->ReadVC.clock = tState->VC.clock;
    } else {
        // DJIT+ Write Check

        bool nowwRace = true, norwRace = true;
        // check for (Write-Write) race and (Write-Read) race
        if(mState->lastWriteTID != tid){
            nowwRace = happensBeforeCheck(mState->WriteVC, tState->VC);
        }
        if(mState->lastReadTID != tid){
            norwRace = happensBeforeCheck(mState->ReadVC, tState->VC);
        }
        if(!nowwRace || !norwRace){
            if(!nowwRace){
                assert(mState->lastWriteTID != tid);
                updateRaceReport(mState->lastWriteIP, ip, addr, "W-W", mState->lastWriteTID, tid);
            }
            if(!norwRace){
                assert(mState->lastReadTID != tid);
                updateRaceReport(mState->lastReadIP, ip, addr, "R-W", mState->lastReadTID, tid);
            }
        }
        mState->lastWriteTID = tid;
        mState->lastWriteIP = ip;
        mState->WriteVC.clock = tState->VC.clock;
    }
}

// ======
// PARSER
// ======

int countThreads(const string& filepath) {
    
    int numThreads = 1;
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Error: Could not open trace file: " << filepath << endl;
        return -1;
    }

    string line;
    while (getline(file, line)) {
        if (line.find("THREAD: Before pthread_create") != string::npos) {
            numThreads++;
        }
    }

    return numThreads;
}

void parseTraceFile(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) {
        cerr << "Error: Could not open trace file: " << filepath << endl;
        return;
    }


    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;

        // Format: MEM: TID: 0, IP: 0x..., ADDR: 0x..., Size (B): 8, isRead: 1
        if (line.find("MEM:") != string::npos) {
            
            TID tid;
            string ip;
            string addr;
            int isReadInt;
            
            size_t pTid = line.find("TID:");
            size_t pIp = line.find("IP:");
            size_t pAddr = line.find("ADDR:");
            size_t pRead = line.find("isRead:");

            if (pTid != string::npos && pAddr != string::npos) {
                tid = stoi(line.substr(pTid + 5, line.find(',', pTid) - (pTid + 5)));
                ip = line.substr(pIp + 4, line.find(',', pIp) - (pIp + 4));
                addr = line.substr(pAddr + 6, line.find(',', pAddr) - (pAddr + 6));
                isReadInt = stoi(line.substr(pRead + 8));

                handleMemoryAccess(tid, ip, addr, (isReadInt == 1));
            }

        } 
        // Format: LOCK: After acquire TID: 0, ADDR: 0x...
        else if (line.find("LOCK: After acquire") != string::npos) {
            
            TID tid;
            string addr;
            
            size_t pTid = line.find("TID:");
            size_t pAddr = line.find("address:");
            
            tid = stoi(line.substr(pTid + 5, line.find(',', pTid) - (pTid + 5)));
            addr = line.substr(pAddr + 9);
            
            handleLockAcquire(tid, addr);

        } 
        // Format: LOCK: Before release TID: 0, ADDR: 0x...
        else if (line.find("LOCK: Before release") != string::npos) {
            TID tid;
            string addr;
            
            size_t pTid = line.find("TID:");
            size_t pAddr = line.find("address:");
            
            tid = stoi(line.substr(pTid + 5, line.find(',', pTid) - (pTid + 5)));
            addr = line.substr(pAddr + 9);
            
            handleLockRelease(tid, addr);
        }
        // Format: THREAD: Child begin: 1
        else if (line.find("THREAD: Child begin") != string::npos) {
            size_t pChild = line.find("begin:");
            TID childTid = stoi(line.substr(pChild + 7));
            handleChildBegin(childTid);
        }
        // Format: THREAD: Child end: 1
        else if (line.find("THREAD: Child end") != string::npos) {
            size_t pChild = line.find("end:");
            TID childTid = stoi(line.substr(pChild + 5));
            handleChildEnd(childTid);
        }
        // THREAD: Before pthread_create() Parent: 0
        else if (line.find("THREAD: Before pthread_create") != string::npos) {
            size_t pParent = line.find("Parent:");
            TID parentTid = stoi(line.substr(pParent + 7, line.find(',', pParent) - (pParent + 7)));
            handleThreadCreate(parentTid);
        }
        // THREAD: Before pthread_join(): Parent: 0
        else if (line.find("THREAD: Before pthread_join") != string::npos) {
            size_t pParent = line.find("Parent:");
            TID parentTid = stoi(line.substr(pParent + 7, line.find(',', pParent) - (pParent + 7)));
            handleThreadJoin(parentTid);
        }
    }
    
    file.close();
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <path_to_trace>" << endl;
        return 1;
    }

    NUM_THREADS = countThreads(argv[1]);
    
    // Initialize the threads vector clock
    Threads[0] = new ThreadState(0);
    
    parseTraceFile(argv[1]);
    printRaceReports();

    return 0;
}