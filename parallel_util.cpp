#include "parallel_util.h"

TaskQueue::TaskQueue() : shutdown(false) {}
TaskQueue::~TaskQueue() {
    while (!tasks.empty()) {
        delete tasks.front();
        tasks.pop();
    }
}

void TaskQueue::enqueue(Task* task) {
    std::lock_guard<std::mutex> lock(mtx);
    tasks.push(task);
    cv.notify_one();
}

Task* TaskQueue::dequeue() {
    std::unique_lock<std::mutex> lock(mtx);
    while (tasks.empty() && !shutdown) {
        cv.wait(lock);
    }
    if (shutdown && tasks.empty()) return nullptr;
    Task* task = tasks.front();
    tasks.pop();
    return task;
}

void TaskQueue::setShutdown() {
    std::lock_guard<std::mutex> lock(mtx);
    shutdown = true;
    cv.notify_all();
}

double Worker::calculateU(double Z){
    return -0.21436 + 1.35034 * log10(Z);
}
double Worker::calculatePso(double Z){
    double U = calculateU(Z);
    double u_pow[9];
    u_pow[0] = 1.0;
    for (int i = 1; i < 9; i++){
        u_pow[i] = u_pow[i - 1] * U;
    }
    double log_pso = 0.0;
    for (int i = 0; i < 9; i++){
        log_pso += c[i] * u_pow[i];
    }
    return pow(10, log_pso);
}

void Worker::run() {
    while (true) {
        Task* task = queue.dequeue();
        if (task == nullptr) break; // Shutdown signal
        int i = task->row_id;
        int t = task->time_step;
        int max_R = t * 343; // Speed of sound
        for (int j = 0; j < N; ++j) {
            if (dist_grid[i][j] <= max_R && grid[i][j] == 0.0) {
                double Z = dist_grid[i][j] / pow(W, 1.0 / 3.0);
                grid[i][j] = calculatePso(Z);
            }
        }
        delete task;
        tasks_complete++;
    }
}

Worker::Worker(int id, TaskQueue& q, double** g, double** d, std::atomic<int>& tc)
        : id(id), queue(q), grid(g), dist_grid(d), tasks_complete(tc) {
        t = std::thread(&Worker::run, this);
    }

Worker::~Worker() {
    t.join();
}