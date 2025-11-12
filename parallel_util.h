#ifndef PARALLEL_UTIL_H
#define PARALLEL_UTIL_H

#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>

const int N = 4000;          
const int cell_size = 10;
const int max_t = 100;
const int middle = N / 2;
const double W = 5000 * 1e6 / 1000.0; 
const double c[] = {
    2.611369, -1.690128, 0.00805, 0.368743, -0.005162,
    -0.08023, 0.00793, -0.004785, 0.000768
};


struct Task {
    int start_row;
    int end_row; // Inclusive end row for batching
    int time_step;
    double max_R;
    double previous_R;
    Task(int start, int end, int t, double max_R, double previous_R) : start_row(start), end_row(end), time_step(t), max_R(max_R), previous_R(previous_R) {}
};

// Task Queue class declaration
class TaskQueue {
private:
    std::queue<Task*> tasks;
    
    bool shutdown;

public:
    std::mutex mtx;
    std::condition_variable cv;
    TaskQueue();
    ~TaskQueue();
    void enqueue(Task* task);
    Task* dequeue();
    void setShutdown();

};

// Worker class declaration
class Worker {
private:
    int id;
    std::thread t;
    TaskQueue& queue;
    double** grid;
    double** dist_grid;

    void run();

public:
    Worker(int id, TaskQueue& q, double** g, double** d);
    ~Worker();
};

#endif // PARALLEL_UTIL_H