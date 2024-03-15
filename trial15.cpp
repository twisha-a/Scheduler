#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <cstring>
using namespace std;
// Scheduler* scheduler;
enum TransitionState {TRANS_TO_READY, TRANS_TO_PREEMPT, TRANS_TO_RUN, TRANS_TO_BLOCK, TRANS_TO_DONE};
enum process_state_t {STATE_CREATED, STATE_READY, STATE_RUNNING, STATE_BLOCKED, STATE_PREEMPT, STATE_DONE} ;
int quantum = 9999; // default quantum
char* scheduler_type = nullptr;
int maxprio=4;
bool isVerbose = false;

// Process class

vector<int> randvals;
int totalRandomValues = 0;
int ofs = 0;

class Event; // Forward declaration

Event* pending_evt;

// int myRandom(int burst) {
//     static int ofs = 1;
//     int res;
//     // wrap around or step to the next
//     if (ofs>randvals[0]){
//         ofs = 1;
//         res = (randvals[ofs++] % burst) + 1;
//     }
//     else{
//         res =(randvals[ofs++] % burst) + 1;
//     }
//     return res;
// }

int myRandom(int burst = 4)
{
    if (ofs == randvals.size())
    {
        ofs = 0;
    }
    return 1 + (randvals[ofs++] % burst);
}


class Process{
    public:
        int pid = 0;
        int AT, TC, CB, IO;
        int state_ts;
        int time_in_prev_state =0;
        process_state_t current_state = STATE_CREATED; // initial state
        int io_burst, cpu_burst, io_time=0, cpu_wait_time=0, cpu_burst_time=0, remaining_cpu_burst_time=0, remaining_io_burst_time=0;
        int static_priority, dynamic_priority;
        bool preempted = false, dynamic_priority_reset=false;
        Event* pending_evt = nullptr;
        int finishingTime;

        
        Process(int process_id, int at, int tc, int cb, int io){
            // cout << "Process called" << endl;
            pid = process_id;
            AT = at;
            TC = tc;
            CB = cb;
            IO = io;
            state_ts = at;
            remaining_cpu_burst_time = tc;
            //time_in_prev_state = 0;
            static_priority = myRandom(maxprio);
            dynamic_priority = static_priority - 1;
            // cout << "Process ID: " << pid << endl;
        }

        void printInfo() const {
            std::cout << "Process ID: " << pid
                    << ", Arrival Time: " << AT
                    << ", Total CPU Time: " << TC
                    << ", CPU Burst: " << CB
                    << ", IO: " << IO
                    << ", State Timestamp: " << state_ts
                    << ", Remaining CPU Burst Time: " << remaining_cpu_burst_time
                    << std::endl;
        }

        void updateIOandPriority() {
            io_time += time_in_prev_state;
            io_burst -= time_in_prev_state;
            dynamic_priority = static_priority - 1;
        }

        void transitionToReady(int currentTime, bool isVerbose) {
            if (current_state == STATE_BLOCKED) {
                if (isVerbose) {
                    cout << currentTime << " " << pid << " " << time_in_prev_state << ": BLOCK -> READY" << endl;
                }
                //cout << "pos5" << endl; 

                // if (CURRENT_IO_PROCESS == process) {
                //     //CURRENT_IO_PROCESS = nullptr;
                //     process->io_time = process->io_time + process->time_in_prev_state;
                //     process->remaining_io_burst_time = 0;
                // }
                
                io_time += time_in_prev_state;
                io_burst -= time_in_prev_state;
                dynamic_priority = static_priority - 1;
                // cout <<"io time"<< process->io_time << endl;
                // cout <<"io burst"<< process->io_burst << endl;
                // cout <<"dynamic_priority"<< process->dynamic_priority << endl;
                // cout <<"time_in_prev_state"<< process->time_in_prev_state << endl;
            } 
            else {
                if (isVerbose) {
                    cout << currentTime << " " << pid << " " << time_in_prev_state << ": CREATED -> READY" << endl;
                }
            }

            
        // Note: This function does not handle adding the process to the scheduler's ready queue
        // or setting CALL_SCHEDULER to true, as these actions involve external objects and states.
        }

        void calculateCPUBurst(int& quantum, bool& isVerbose) {
            if (!preempted) {
                int cb_rand = myRandom(CB);
                cpu_burst = std::min(cb_rand, remaining_cpu_burst_time);
                if (isVerbose) {
                    cout << "Calculating CPU burst: " << cpu_burst << " (non-preempted)" << endl;
                }
            } else {
                // Process was preempted - it was interrupted before completing its last CPU burst
                // This condition might typically reset the preempted flag or handle related logic
                preempted = false;
                if (isVerbose) {
                    cout << "Process was preempted, continuing previous CPU burst" << endl;
                }
            }
        }

        void updateIOTimeRange(vector<pair<int, int>>& IOUse, int currentTime) {
            // Assuming io_burst has already been determined
            int startTime = currentTime;
            int endTime = currentTime + io_burst;

            // Adding the time range to the given vector
            IOUse.push_back(make_pair(startTime, endTime));
        }
       
        
};
Process* CURRENT_IO_PROCESS = nullptr;

// Event class
class Event{
    public:
        int time_stamp;
        int process_id;
        Process *event_process;
        TransitionState transition;
        static int global_id;
        bool remove = false;

        Event(int time, Process* process, TransitionState TransitionTo){
            // cout<< "Event called 2: " << endl;
            time_stamp = time;
            // cout<< "Event called 2 TIME : " << time_stamp << endl;

            event_process = process;
            // cout<< "Event called 2 event_process : " << event_process << endl;

            transition = TransitionTo;
            // cout<< "Event called 2 transition : " << transition << endl;
            
            // event_process->pid = global_id++;
            // cout<< "Event called 2 event_process->pid : " << event_process->pid << endl;

            // cout<< "Transition State: " << transition << endl;
        }
        // bool CompareEvent(Event e1, Event e2){
        //     if (e1.time_stamp == e2.time_stamp){
        //         return e1.event_process->pid < e2.event_process->pid;
        //     }
        //     return e1.time_stamp < e2.time_stamp;
        // }

        struct CompareEvent {
        bool operator()(const Event* e1, const Event* e2) const {
            if (e1->time_stamp == e2->time_stamp) {
                return e1->event_process->pid > e2->event_process->pid;
            }
            return e1->time_stamp > e2->time_stamp;
        }
    };
};
// DES Layers
int Event::global_id = 0;
class DES_Layers{
    public:
        priority_queue<Event, vector<Event*>, Event::CompareEvent> event_queue;
        // put event
        void put_event(Event* event){
            event_queue.push(event);
            // cout << *event << endl; 
            // while (!event_queue.empty()) {
            //     Event* event = event_queue.top();
            //     cout << *event << endl; // Requires Event to have an overloaded << operator
            //     // If you only have a toString method in Event, use:
            //     // std::cout << event->toString() << std::endl;
            // }       

        }
        // remove event
        Event* remove_event(){
            if (!event_queue.empty()){
                Event* next_event = event_queue.top();
                event_queue.pop();
                return next_event;
            }
            else{
                return NULL;
            }
        }
        // get next event
        Event* get_next_event(){
            if (!event_queue.empty()){
                return event_queue.top();
            }
            else{
                return NULL;
            }
        }

};

// Scheduler class
class Scheduler; // Forward declaration

Scheduler* scheduler;

class Scheduler{
    public:
    int quantum=10000;
    int maxprio=4;
    
    deque<Process*> ready_queue;
    virtual void add_process(Process* process){
        // add process to the scheduler
        // cout<<"entered add process"<<endl;
        ready_queue.push_front(process);
        // cout << "Process added to the scheduler" << endl;

    }
    virtual Process* get_next_process(){
        if (ready_queue.empty()){
            return NULL;
        }  
        Process* top_process = ready_queue.front();
        ready_queue.pop_front();
        return top_process;
    }
    virtual bool no_Process(){
        // cout<<"entered no process"<<endl;
        return ready_queue.empty();
    }
};

class FCFS: public Scheduler{
    
    public:
        deque<Process*> ready_queue;
        void add_process(Process* process) override {
            // cout<< "entered FCFS add process"<<endl;
            ready_queue.push_back(process);
            // cout << "Process added to the scheduler" << endl;
        }
        Process* get_next_process() override {
            Process* top_process = ready_queue.front();
            ready_queue.pop_front();
            return top_process;
        }
        bool no_Process() override {
        return ready_queue.empty();
        }
};
class LCFS: public Scheduler{
    public:
        deque<Process*> ready_queue;
        void add_process(Process* process) override {
            // cout<< "entered LCFS add process"<<endl;

            ready_queue.push_back(process);
        }
        Process* get_next_process() override{
            Process* top_process = ready_queue.back();
            ready_queue.pop_back();
            return top_process;
        }
        bool no_Process() override {
        return ready_queue.empty();
        }
};
class SRTF: public Scheduler {
private:
    static bool compare(Process* p1, Process* p2) {
        if (p1->remaining_cpu_burst_time == p2->remaining_cpu_burst_time) return p1 > p2;
        return p1->remaining_cpu_burst_time > p2->remaining_cpu_burst_time;
    }

public:
    /// make this deque
    priority_queue<Process*, vector<Process*>, decltype(&SRTF::compare)> ready_queue{SRTF::compare};

    void add_process(Process* proc) override {
        // cout<< "entered srtf add process"<<endl;
        ready_queue.push(proc);
    }

    Process* get_next_process() override {
        if (ready_queue.empty()) {
            return NULL;
        }
        Process* temp = ready_queue.top();
        ready_queue.pop();
        return temp;
    }
    bool no_Process() override {
        return ready_queue.empty();
    }

};

class RR: public Scheduler {
private:
    queue<Process*> ready_queue;
public:
    RR(int q) : Scheduler() {
        quantum = q;
    }
    void add_process(Process* proc) override {
        // cout<< "entered RR add process"<<endl;
        ready_queue.push(proc);
    }
    Process* get_next_process() override {
        if (ready_queue.empty()) {
            return NULL;
        }
        Process* temp = ready_queue.front();
        ready_queue.pop();
        return temp;
    }
    bool no_Process() override {
        return ready_queue.empty();
    }
};
// class PRIO {
// public:
//     int quantum;
//     int maxprio;
//     PRIO(int q, int mp) : quantum(q), maxprio(mp) {}
// };
class PRIO: public Scheduler {
    int quantum;
    int maxprio;
    int AQCount = 0, EQCount = 0;
    vector<queue<Process*>> activeQueue;
    vector<queue<Process*>> expiredQueue;

public:
    PRIO(int q, int mp) : quantum(q), maxprio(mp), activeQueue(vector<queue<Process*>>(mp)), expiredQueue(vector<queue<Process*>>(mp)) {}

    void add_process(Process* process) override {
        // cout<< "entered PRIO and PrePRIO add process"<<endl;
        if (process->current_state == STATE_CREATED || process->current_state == STATE_BLOCKED) {
            activeQueue[process->dynamic_priority].push(process);
            AQCount++;
        } else if (process->current_state == STATE_RUNNING) {
            if (!process->dynamic_priority_reset) {
                activeQueue[process->dynamic_priority].push(process);
                AQCount++;
            } else {
                expiredQueue[process->dynamic_priority].push(process);
                EQCount++;
            }
            process->dynamic_priority_reset = 0;
        }
    }

    void ActExpExchange() {
        if (AQCount != 0) return;
        activeQueue.swap(expiredQueue);
        swap(AQCount, EQCount);
    }

    Process* get_next_process() override {
        if (AQCount == 0) ActExpExchange();
        for (int prio = maxprio - 1; prio >= 0; prio--) {
            if (!activeQueue[prio].empty()) {
                Process* process = activeQueue[prio].front();
                activeQueue[prio].pop();
                AQCount--;
                return process;
            }
        }
        return nullptr;
    }

    bool no_Process() override {
        if (AQCount == 0) ActExpExchange();
        return AQCount == 0 && EQCount == 0;
    }
};
class PREPRIO {
public:
    int quantum;
    int maxprio;
    PREPRIO(int q, int mp) : quantum(q), maxprio(mp) {}
};
    
// Read input file
void ReadInputFile(string filename, vector<Process*>& processes){
    ifstream file(filename);
    string line;
    // cout << "Reading input file" << endl;
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
    }
    int pid = 0;
    int AT, TC, CB, IO;
    while (getline(file, line)) {
        istringstream inputData(line); // Use istringstream and prefix with 
        // cout << "Reading input file still" << endl;
        inputData >> AT >> TC >> CB >> IO;
        processes.push_back(new Process(pid++, AT, TC, CB, IO));
        // cout << "Reading input file still" << endl;

        // Optionally, process or print the values here
        // For example, print them to verify they're being read correctly:
        //cout << "AT: " << AT << ", TC: " << TC << ", CB: " << CB << ", IO: " << IO << endl;
    }
    // cout << "Reading input file end" << endl;

    file.close();

    }

// Read random file 
// vector<int> randvals;
// int totalRandomValues = 0;
// int ofs = 0;
// void ReadRandomFile(const string& randfile){
//     ifstream randfil(randfile);
//     int temp;
//       randfil >> temp;
//       randvals.push_back(temp);
//       for (int a = 1; a < randvals[0]; a++) {
//           randfil >> temp;
//           randvals.push_back(temp);
//       }
//     // cout << "readRandomfile done: " << endl; 
// };
int getToken(string line, bool firstChar)
{
    char *cstr = new char[line.length() + 1];
    strcpy(cstr, line.c_str());
    char *res = strtok((firstChar ? cstr : NULL), " \t\n");
    return stoi(res);
}
ifstream randfile;
void ReadRandomFile()
{
    bool firstNum = true;
    string line;
    int size, i = 0;
    
    while (getline(randfile, line))
    {
        if (firstNum)
        {
            size = getToken(line, true);
            firstNum = false;
            randvals.resize(size);
        }
        else
        {
            randvals.insert(randvals.begin() + i, getToken(line, true));
            i++;
        }
    }
}





// simulation
void simulation(DES_Layers *des_layers, vector<pair<int, int> > &IOUse){
    Event* evt;
    Scheduler* scheduler;
    bool CALL_SCHEDULER;
    Process* CURRENT_RUNNING_PROCESS = nullptr;
    // cout << "ithe: " << scheduler->quantum << endl;
    int quantum, maxprio;
    if (strcmp(scheduler_type, "F") == 0) {
    scheduler = new FCFS();
    cout << "FCFS" << "\n";
    } else if (strcmp(scheduler_type, "L") == 0) {
        scheduler = new LCFS();
        cout << "LCFS" << "\n";
    } else if (strcmp(scheduler_type, "S") == 0) {
        scheduler = new SRTF();
        cout << "SRTF" << "\n";
    } else if (*scheduler_type == 'R') {
        quantum = atoi(scheduler_type + 1);
        scheduler = new RR(quantum);
        cout << "RR" << " " << scheduler->quantum << "\n";
    } else if (*scheduler_type == 'P') {
        sscanf(scheduler_type + 1, "%d:%d", &quantum, &maxprio);
 //cout<<"i"<<endl;       
        scheduler = new PRIO(quantum, maxprio);
        cout << "PRIO" << " " << scheduler->quantum << "\n";
    } else if (*scheduler_type == 'E') {
        maxprio = 4;
        sscanf(scheduler_type + 1, "%d:%d", &quantum, &maxprio);
        scheduler = new PRIO(quantum, maxprio);
        
        cout << "PREPRIO" << " " << scheduler->quantum << scheduler->maxprio <<"\n";
    }

    // cout<<"read scheduler type "   << endl;    
    
    //char* scheduler_type = ReadSchedulerType(argc, argv);
    Event* newEvent;
    // ReadSchedulerType schedulerType = 
    // cout << "yeyyy entered simulation " << endl;
    
    while (evt = des_layers->get_next_event()){
        // get next event
        // cout << " entered while - pos 1  " << evt << endl;
        // cout << des_layers->event_queue.size() << endl;
        // for (int i = 0; i < des_layers->event_queue.size(); i++)
        // {
            // cout << "> " << des_layers->event_queue.top()->event_process->remaining_cpu_burst_time << endl;
            //cout << "> " <<evt->transition<<endl;
            // cout << ">> " << TRANS_TO_PREEMPT<<endl;
        // }
        

        evt = des_layers->remove_event();
        if (evt-> remove == true){
            continue;
        }
        // cout << " entered while - pos 2  " << endl;


        Process* process = evt->event_process;
        // cout << " entered while - pos 3 process  " << process << endl;

        int CURRENT_TIME = evt->time_stamp;
        TransitionState transition = evt->transition;
        process->time_in_prev_state = evt->time_stamp - process->state_ts;
        // cout << "After calculation - time in prev state  " << process->time_in_prev_state << endl;
        
        // process->state_ts = evt->time_stamp;
        // CURRENT_TIME = evt->time_stamp;
        process->state_ts = CURRENT_TIME;

        //TRANS_TO_READY
        if (transition == TRANS_TO_READY) {
            
            // cout << " entered trans to ready " << endl;

            if (process->current_state == STATE_BLOCKED) {
                if (isVerbose)
                {
                    cout << CURRENT_TIME << " " << process->pid << " " << process->time_in_prev_state << ": BLOCK -> READY" << endl;
                } 
                //cout << "pos5" << endl; 

                // if (CURRENT_IO_PROCESS == process) {
                //     //CURRENT_IO_PROCESS = nullptr;
                //     process->io_time = process->io_time + process->time_in_prev_state;
                //     process->remaining_io_burst_time = 0;
                // }
                process->updateIOandPriority();
                // cout <<"io time"<< process->io_time << endl;
                // cout <<"io burst"<< process->io_burst << endl;
                // cout <<"dynamic_priority"<< process->dynamic_priority << endl;
                // cout <<"time_in_prev_state"<< process->time_in_prev_state << endl;
            }
            else{
                if (isVerbose)
                {
                    cout << CURRENT_TIME << " " << process->pid << " " << process->time_in_prev_state  << ": CREATED -> READY" << endl;
                }
            }
            // else{
                // cout << "pos6" << endl;
                scheduler->add_process(process); 
                // cout << "pos7" << endl;
                process->current_state = STATE_READY;
                CALL_SCHEDULER = true;
            // }

        }
        // TRANS_TO_RUN
        else if (transition == TRANS_TO_RUN) {
            // cout << "entered trans to run" << endl;
            // cout<< "Print Preemted staus : " << process->preempted << endl;
            process->calculateCPUBurst(quantum, isVerbose);
            // cout<< "Print Preemted staus after setting to false: pos 1 : " << process->preempted << endl;
            CURRENT_RUNNING_PROCESS = process;
            //int count_wait;
            if(process->current_state == STATE_READY){

                //process->cpu_wait_time = process->cpu_wait_time + CURRENT_TIME;
                process->cpu_wait_time = process->cpu_wait_time + process->time_in_prev_state;
                // count_wait++;
                // cout<< "count if wait time : " << count_wait << endl;
                // cout<< "count if wait time : " << count_wait << endl;
                //cout<<"TRANS_TO_RUN: CPU WAIT TIME: " << process->cpu_wait_time << " TIME IN PREV STATE: " << process->time_in_prev_state << endl;
            }
            process->current_state = STATE_RUNNING;
            // cout<< "trans to run Quantum: " << quantum << endl;
            // cout<< "trans to run burst: " << process->cpu_burst << endl;
            //                 cout << "= " <<  process -> remaining_cpu_burst_time <<endl;
            //if (process->remaining_cpu_burst_time < 0) exit(1);
            // 8 <= 4 -> false go to else
            if (process->cpu_burst <= quantum){
                if (process -> remaining_cpu_burst_time <= process-> cpu_burst){
                    newEvent = new Event(CURRENT_TIME+ process -> remaining_cpu_burst_time, process, TRANS_TO_DONE);
                }
                else{
                    newEvent = new Event(CURRENT_TIME+ process->cpu_burst, process, TRANS_TO_BLOCK);
                    
                    }
                // cout<< "Print Preemted staus after setting to false: pos 2 : " << process->preempted << endl;

            }
            else{
                newEvent = new Event(CURRENT_TIME+ quantum, process, TRANS_TO_PREEMPT);
                process->preempted = 1;
                // cout<< "Print Preemted staus after setting to false: pos 3 : " << process->preempted << endl;

            }
            process->pending_evt = newEvent;
            des_layers->put_event(newEvent);
            // cout<< "Print Preemted staus after setting to false: pos 4 : " << process->preempted << endl;

        } 
        // TRANS_TO_PREEMPT
        else if (transition == TRANS_TO_PREEMPT) {
            // cout << "entered preemption" << endl;
            process -> remaining_cpu_burst_time = process -> remaining_cpu_burst_time - process->time_in_prev_state;
            //process ->cpu_burst_time = process ->cpu_burst_time - time_in_prev_state;
            process-> cpu_burst = process-> cpu_burst - min(quantum, process->time_in_prev_state);
            CURRENT_RUNNING_PROCESS = nullptr;
            process-> dynamic_priority -= 1;
            
            if (process-> dynamic_priority != -1){
                process->dynamic_priority_reset = false;
            }
            else{
                process->dynamic_priority = process->static_priority - 1;
                process->dynamic_priority_reset = true;
            }
            if (isVerbose)
            {
                cout << CURRENT_TIME << " " << process->pid << " " << process->time_in_prev_state << ": RUNNG -> READY"
                     << " cb=" << process->remaining_cpu_burst_time << " rem=" << process->cpu_burst << " prio=" << process->dynamic_priority << endl;
            }
            scheduler->add_process(process);
            process->current_state = STATE_READY;
            CALL_SCHEDULER = true;
        } 
        // TRANS_TO_BLOCK
        else if (transition == TRANS_TO_BLOCK) {
            // cout << "entered block" << endl;
            CURRENT_RUNNING_PROCESS = nullptr;
            process->io_burst = myRandom(process->IO);
            

            process ->cpu_burst= process->cpu_burst - process->time_in_prev_state;
            process->remaining_cpu_burst_time = process->remaining_cpu_burst_time - process->time_in_prev_state;
            process->current_state = STATE_BLOCKED;
            
            process->updateIOTimeRange(IOUse, CURRENT_TIME);
            if (isVerbose)
            {
                cout << CURRENT_TIME << " " << process->pid << " " << process->time_in_prev_state << ": RUNNG -> BLOCK"
                        << " ib=" << process->io_burst << " rem=" << process->cpu_burst << endl;
            }
            newEvent = new Event(CURRENT_TIME + process->io_burst, process, TRANS_TO_READY);
            des_layers->put_event(newEvent);
            process->pending_evt = newEvent;
            CALL_SCHEDULER = true;
        } 
        else if(transition == TRANS_TO_DONE){
            //cout<< "entered done" << endl;
            process->remaining_cpu_burst_time = 0;
            process->current_state = STATE_DONE;
            CURRENT_RUNNING_PROCESS = nullptr;
            process->finishingTime = CURRENT_TIME;
            CALL_SCHEDULER = true;
        }
        else {
            cout << "in class simulation transtion cases - Invalid Transition" << endl;
            // Handle unknown transition
        }
        delete evt;
        // cout<< "Event deleted" << endl;
        evt = nullptr;

        if (CALL_SCHEDULER){
            // cout<< "pos 1 -call scheduler entered" << endl;
            if (CURRENT_RUNNING_PROCESS != nullptr && scheduler_type == "E" && CURRENT_RUNNING_PROCESS->dynamic_priority < process->dynamic_priority && CURRENT_RUNNING_PROCESS->pending_evt->time_stamp != CURRENT_TIME){
                // cout <<"pos 2 call scheduler"<<endl;
                //cout<< "call scheduler entered pos 1- scheduler type: "<< scheduler_type << endl;
                CURRENT_RUNNING_PROCESS->pending_evt->remove = true;
                Event* newEvent = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TRANS_TO_PREEMPT);
                des_layers->put_event(newEvent);
                // cout << "preempted" << endl;
                // exit(1);
                CURRENT_RUNNING_PROCESS->preempted = true;
                continue;
            }
            //cout << "call scheduler else pos 2" << endl;
            Event* next_event = des_layers->get_next_event();
            // cout <<"pos 3 call scheduler"<<endl;

            if (next_event != nullptr && next_event->time_stamp == CURRENT_TIME){
            // cout << "call scheduler  pos 4" << endl;
                continue;
            }
            //cout << "call scheduler pos 5" << endl;

            CALL_SCHEDULER = false;
            // cout<< "CURRENT RUNNING PROCESS : "<<CURRENT_RUNNING_PROCESS << endl;
            // if (CURRENT_RUNNING_PROCESS == nullptr){cout << "CURRENT_RUNNING_PROCESS == nullptr"  << endl;}
            // else{cout << "CURRENT_RUNNING_PROCESS != nullptr"  << endl;}
            //if (!scheduler->no_Process()){cout << "!scheduler->no_Process()"  << endl;}
            // else{cout << "scheduler->no_Process()"  << endl;}
            //cout << "scheduler->no_Process(): True" << !scheduler->no_Process() << endl;
            if (CURRENT_RUNNING_PROCESS == nullptr && !scheduler->no_Process()){
                // cout << "call scheduler  pos 5" << endl;

                CURRENT_RUNNING_PROCESS = scheduler->get_next_process();
                // cout<< "CURRENT RUNNING PROCESS : "<<CURRENT_RUNNING_PROCESS << endl;
                // cout<< "CURRENT TIME : "<<CURRENT_TIME << endl;

                Event* newEvent = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TRANS_TO_RUN);
                // cout<< "EVENT READ " << endl;

                des_layers->put_event(newEvent);
                process->pending_evt = newEvent;

            }
            // cout << "exiting call scheduler else pos 6" << endl;

        }
        // cout << "call scheduler  pos 7" << endl;
        
    };
    delete scheduler;
    // cout << "call scheduler  pos 8" << endl;

};

//print

int main(int argc, char *argv[]) {
    vector<Process*> processes;
    // DES_Layers des_layers;
    string inputFileName, randFileName;
    //char* schedulerType = nullptr;

    int opt;
    int quantum = 10; // default quantum, this can be parsed from args
    int maxprio = 4; // default max priority, parse as needed
    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
            case 'v' :
            isVerbose = true;
            break;
            case 's':
                scheduler_type = optarg;
                sscanf(scheduler_type, "%d:%d", &quantum, &maxprio);
                break;
            default: 
                fprintf(stderr, "Usage: %s [-s schedulerType] inputfile randfile\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        inputFileName = argv[optind];
        randFileName = argv[optind + 1]; // Assuming rand file is the second argument
        //cout << "Hello: " << endl;

    } else {
        // cout << "Expected argument after options\n";
        cerr << "Expected argument after options\n";
        exit(EXIT_FAILURE);
    }

    //cout << "Scheduler Type: " << schedulerType << "\n";
    //cout << "Input File: " << inputFileName << "\n";
    //cout << "Rand File: " << randFileName << "\n";
    
    // Now you can call ReadInputFile, ReadRandomFile, and then simulate
    randfile.open(randFileName);
    ReadRandomFile();
    ReadInputFile(inputFileName, processes);


    //cout << "entering des layers" << endl;
    DES_Layers* desLayer = new DES_Layers();
    //cout << "exited des layers" << endl;
    Event *newEvt;
    if(processes.empty()){
        cout << "No processes to simulate" << endl;
        return 0;
    }
    // else{
    //     cout << "processes" << processes.size() << endl;
    // }
    for (int i = 0; i<processes.size(); ++i){
        newEvt = new Event(processes[i]->AT, processes[i], TRANS_TO_READY);
        desLayer->put_event(newEvt);
        //cout << "Event going in" << newEvt << endl;
        //cout << "Event going in - AT" << processes[i]->AT << endl;
        //cout << "Event going in - Entire Process" << processes[i] << endl;
        

        processes[i]->pending_evt = newEvt;
    }

    vector<pair<int, int> > IOUse; //[IOstart, IOend]
      // start simulation
    
    cout << "" << endl;
    //Scheduler* scheduler = ReadSchedulerType(argc, argv);
    simulation(desLayer, IOUse);
    //cout << "exiting simulation" << endl;

    int lastFT = 0;
    double cpuUti=0, ioUti, avgTT, avgCW=0, throughput;
    // print out statistics
    for (int i = 0; i< processes.size(); i++){
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
        i, processes[i]->AT, processes[i]->TC, processes[i]->CB, processes[i]->IO,
        processes[i]->static_priority, processes[i]->finishingTime, 
        processes[i]->finishingTime-processes[i]->AT, processes[i]->io_time, 
        processes[i]->cpu_wait_time);
        lastFT=max(lastFT, processes[i]->finishingTime);
        cpuUti += processes[i]->TC;
        ioUti += processes[i]->io_time;
        avgTT += processes[i]->finishingTime - processes[i]->AT;

        avgCW += processes[i]->cpu_wait_time;
    }

    cpuUti = 100 * cpuUti / lastFT;
    
    std::sort(IOUse.begin(), IOUse.end());
    int totalIOUse = 0;
    if (!IOUse.empty()) {
        int start = IOUse[0].first, end = IOUse[0].second;
        for(auto x = IOUse.begin(); x!=IOUse.end(); ++x){
        if (end<x->first){
            totalIOUse += end - start;
            start = x->first;
            end = x->second;
        }
        else end = max(end, x->second);
        }
        totalIOUse += end - start;
    }
    ioUti = 100.0 * totalIOUse / lastFT;

    double count = processes.size();
    // cout<< "count: " << count << endl;
    avgTT /= count;
    // cout<< "avgTT: " << avgTT << endl;
    avgCW /= count;
    // avgcw = /2
    // cout << "lastFT: " << lastFT << endl;

    throughput = 100.0 * processes.size() / lastFT;
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", 
        lastFT, cpuUti, ioUti, avgTT, avgCW, throughput);
    return 0;
    // Cleanup
    for (auto p : processes) {
        delete p;
    }

    return 0;
}
