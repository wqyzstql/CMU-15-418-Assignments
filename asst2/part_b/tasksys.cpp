#include "tasksys.h"

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
    return 0;
}

void TaskSystemSerial::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    this->num_threads_ = num_threads;
    threads_pool_ = new std::thread[num_threads];
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {
    delete[] threads_pool_;
}

void TaskSystemParallelSpawn::threadRun(IRunnable* runnable, int num_total_tasks, std::mutex* mutex, int* curr_task) {
    int turn = -1;
    while (turn < num_total_tasks) {
        mutex->lock();
        turn = *curr_task;
        *curr_task += 1;
        mutex->unlock();
        if (turn >= num_total_tasks) {
            break;
        }
        runnable->runTask(turn, num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::mutex* mutex = new std::mutex();
    int* curr_task = new int;
    *curr_task = 0;
    for (int i = 0; i < num_threads_; i++) {
        threads_pool_[i] = std::thread(&TaskSystemParallelSpawn::threadRun, this, runnable, num_total_tasks, mutex, curr_task);
    }
    for (int i = 0; i < num_threads_; i++) {
        threads_pool_[i].join();
    }
    delete curr_task;
    delete mutex;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */
TasksState::TasksState() {
    mutex_ = new std::mutex();
    finished_ = new std::condition_variable();
    finishedMutex_ = new std::mutex();
    runnable_ = nullptr;
    finished_tasks_ = -1;
    left_tasks_ = -1;
    num_total_tasks_ = -1;
}

TasksState::~TasksState() {
    delete mutex_;
    delete finished_;
    delete finishedMutex_;
}

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}


TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    state_ = new TasksState;
    killed = false;
    threads_pool_ = new std::thread[num_threads];
    num_threads_ = num_threads;
    for (int i = 0; i < num_threads; i++) {
        threads_pool_[i] = std::thread(&TaskSystemParallelThreadPoolSpinning::spinningThread, this);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    killed = true;
    for (int i = 0; i < num_threads_; i++) {
        threads_pool_[i].join();
    }
    delete[] threads_pool_;
    delete state_;
}

void TaskSystemParallelThreadPoolSpinning::spinningThread() {
    int id;
    int total;
    while (true){
        if (killed) break;
        state_->mutex_->lock();
        total = state_->num_total_tasks_;
        id = total - state_->left_tasks_;
        if (id < total) state_->left_tasks_--;
        state_->mutex_->unlock();
        if (id < total) {
            state_->runnable_->runTask(id, total);
            state_->mutex_->lock();
            state_->finished_tasks_++;
            if (state_->finished_tasks_ == total) {
                state_->mutex_->unlock();
                // this lock is necessary to ensure the main thread has gone to sleep
                state_->finishedMutex_->lock();
                state_->finishedMutex_->unlock();
                state_->finished_->notify_all();
            }else {
                state_->mutex_->unlock();
            }
        }
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> lk(*(state_->finishedMutex_));
    state_->mutex_->lock();
    state_->finished_tasks_ = 0;
    state_->left_tasks_ = num_total_tasks;
    state_->num_total_tasks_ = num_total_tasks;
    state_->runnable_ = runnable;
    state_->mutex_->unlock();
    state_->finished_->wait(lk); 
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }    
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}
Task::Task(TaskID id, int in){
    this->id=id;
    this->in=in;
    total_tasks_=finished_tasks_=left_tasks_=0;
    finished=false;
}
Task::Task(){

}
Task::~Task(){

}
TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    state_ = new TasksState;
    killed = false;
    cnt=0;
    selected=-1;
    threads_pool_ = new std::thread[num_threads];
    num_threads_ = num_threads;
    hasTasks = new std::condition_variable();
    hasTasksMutex = new std::mutex();
    v=std::vector<Task>(65536);
    for (int i = 0; i < num_threads; i++) {
        threads_pool_[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::sleepingThread, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    cnt=0;
    killed = true;
    for (int i = 0; i < num_threads_; i++) {
        hasTasks->notify_all();
    }
    for (int i = 0; i < num_threads_; i++) {
        threads_pool_[i].join();
    }
    delete[] threads_pool_;
    delete state_;
    delete hasTasks;
    delete hasTasksMutex;
}

void TaskSystemParallelThreadPoolSleeping::sleepingThread() {
    int id;
    int total;
    while (true){
        if (killed) break;
        bool have_task=1;
        state_->mutex_->lock();
        if(selected==-1){
            if(qu.empty()){
                have_task=0;
                bool fi=1;
                for(int i=0;i<cnt;i++)  if(v[i].finished==0){
                    fi=0;
                    break;
                }
                if(fi){
                    state_->finishedMutex_->lock();
                    state_->finishedMutex_->unlock();
                    state_->finished_->notify_all();
                }
            }else {
                Task temp=qu.front();
                qu.pop_front();
                selected=temp.id;
                state_->num_total_tasks_=v[selected].total_tasks_;
                state_->left_tasks_=state_->num_total_tasks_;
                state_->finished_tasks_=0;
                state_->runnable_=v[selected].runnable_;
            }
        }
        //std::cout<<selected<<" "<<std::this_thread::get_id()<<std::endl;
        state_->mutex_->unlock();
        if(have_task==0){
            std::unique_lock<std::mutex> lk(*hasTasksMutex);
            hasTasks->wait(lk);
        }else {
            state_->mutex_->lock();
            total=state_->num_total_tasks_;
            id=total-state_->left_tasks_;
            if(id<total)    state_->left_tasks_--;
            state_->mutex_->unlock();
            if(id<total){
                state_->runnable_->runTask(id, total);
                state_->mutex_->lock();
                state_->finished_tasks_++;
                //std::cout<<state_->finished_tasks_<<" "<<std::this_thread::get_id()<<std::endl;
                if(state_->finished_tasks_==total){
                    //std::cout<<selected<<std::endl;
                    v[selected].finished=1;
                    Task temp=v[selected];
                    for(auto &it:temp.to){
                        v[it].in--;
                        if(v[it].in==0)
                            qu.push_back(v[it]);
                    }
                    for(int i=0; i<num_threads_; i++) 
                        hasTasks->notify_all();
                    selected=-1;
                    state_->mutex_->unlock();
                }else {
                    state_->mutex_->unlock();
                }
            }else {
                std::unique_lock<std::mutex> lk(*hasTasksMutex);
                hasTasks->wait(lk);
            }
        }
    }
}
void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }    
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    //
    // TODO: CS149 students will implement this method in Part B.
    //
    TaskID ID=cnt++;
    int realin=0;
    Task temp = Task(ID, deps.size());
    temp.total_tasks_=num_total_tasks;
    temp.finished_tasks_=0;
    temp.left_tasks_=num_total_tasks;
    temp.runnable_=runnable;
    state_->mutex_->lock();
    for(auto &j:deps){
        if(v[j].finished==1)    continue;
        v[j].to.push_back(ID);
        realin++;
    }
    temp.in=realin;
    v[ID]=temp;
    if(temp.in==0){
        qu.push_back(temp); 
        for(int i=0; i<num_threads_; i++) 
            hasTasks->notify_all();
        //std::cout<<ID<<std::endl;
    }
    state_->mutex_->unlock();
    // for(int i=0;i<=ID;i++)
    //     printf("%d %d || ",v[i].id, v[i].finished);
    // puts("");
    return ID;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //
    // for(int i=0;i<cnt;i++)
    //     printf("%d %d || \n",v[i].id, v[i].finished);
    std::unique_lock<std::mutex> lk(*(state_->finishedMutex_));
    state_->finished_->wait(lk);
    return;
}