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
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
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
    this->threads_num=num_threads;
    threads_pool=std::vector<std::thread>(num_threads);
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {

}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::mutex* mutex=new std::mutex();
    int* curr_task=new int;
    *curr_task=0;                                       
    for(int i=0; i<threads_num;i++){
        threads_pool.emplace_back([=]{
            int turn=-1;
            while(turn<num_total_tasks){
                mutex->lock();
                turn=*curr_task;
                *curr_task+=1;
                mutex->unlock();
                if(turn >= num_total_tasks) break;
                runnable->runTask(turn,num_total_tasks);
            }
        });
    }
    for(auto &it : threads_pool)
        if(it.joinable())
            it.join();
    delete mutex;
    delete curr_task;
    return; 
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

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
    kill=0;
    threads_num=num_threads;
    mutex = new std::mutex();
    finished_ = new std::condition_variable();
    runnable_ = nullptr;
    num_total_tasks_ = 0;
    num_now_ = 0;
    finish_task=0; 
    run_lock_=new std::mutex();
    threads_pool=std::vector<std::thread>(num_threads);
    for(int i=0;i<num_threads;i++){
        threads_pool[i]=std::thread([=]{
            while(1){
                if(kill==1)   break;
                int fetch_task=-1;
                mutex->lock();
                fetch_task=num_now_;
                if(num_now_<num_total_tasks_)
                    num_now_++;
                mutex->unlock();
                if(fetch_task<num_total_tasks_){
                    runnable_->runTask(fetch_task, num_total_tasks_);
                    mutex->lock();
                    finish_task++;
                    if(finish_task==num_total_tasks_){// this if must be included in mutex
                        run_lock_->lock();
                        run_lock_->unlock();//make sure that main thread has locked finished_
                        finished_->notify_all();
                        mutex->unlock();    
                    }else 
                        mutex->unlock();
                }
            }
        });
    }
}
TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning(){
    kill=1;
    for(auto &it:threads_pool)  
        if(it.joinable())   it.join();
    delete mutex;
    delete run_lock_;
    delete finished_;
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    std::unique_lock<std::mutex> run_lock(*run_lock_);
    mutex->lock();
    runnable_=runnable;
    num_total_tasks_=num_total_tasks;
    num_now_=0;
    finish_task=0; 
    mutex->unlock();
    finished_->wait(run_lock);  
    return;
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
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

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
