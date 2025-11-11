#ifndef PARALLEL_UTIL_H
#define PARALLEL_UTIL_H

#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <cmath>
#include <condition_variable>
#include <chrono>
#include <stdio.h>
#include <atomic>
const int N = 4000;
const int cell_size = 10;
const int max_t = 100;
const int middle = N / 2;
const double W = 5000 * 1e6;

const double c[] = {
    2.611369,
    -1.690128,
    0.00805,
    0.336743,
    -0.005162,
    -0.080923,
    -0.004785,
    0.007930,
    0.000768
};

// Task structure for work pool
struct Task {
    int row_id;
    int time_step;
    Task(int row, int t) : row_id(row), time_step(t) {}
};

// Task Queue class declaration
class TaskQueue {
private:
    std::queue<Task*> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool shutdown;

public:
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
    std::atomic<int>& tasks_complete;
    void run();
    double calculateU(double Z);
    double calculatePso(double Z);
public:
    Worker(int id, TaskQueue& q, double** g, double** d, std::atomic<int>& tc);
    ~Worker();
};

#endif // PARALLEL_UTIL_H