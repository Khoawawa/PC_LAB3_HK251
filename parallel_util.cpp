#include "parallel_util.h"
#include <cmath>



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

void Worker::run() {
    while (true) {
        Task* task = queue.dequeue();
        if (task == nullptr) break;
        int t = task->time_step;
        double previous_R = task->previous_R;
        double max_R = task->max_R;
        for (int i = task->start_row; i <= task->end_row; ++i) {
            for (int j = 0; j < N; ++j) {
                double R = dist_grid[i][j];
                if (previous_R < R && R <= max_R && grid[i][j] == 0.0) {
                    double Z = dist_grid[i][j] / pow(W, 1.0 / 3.0);
                    double U = (Z <= 0) ? -1.0 : -0.21436 + 1.35034 * log10(Z);
                    double u_pow[9] = {1.0};
                    for (int k = 1; k < 9; ++k) u_pow[k] = u_pow[k-1] * U;
                    double log_pso = 0.0;
                    for (int k = 0; k < 9; ++k) log_pso += c[k] * u_pow[k];
                    grid[i][j] = pow(10, log_pso);
                }
            }
        }
        delete task;
    }
}

Worker::Worker(int id, TaskQueue& q, double** g, double** d)
    : id(id), queue(q), grid(g), dist_grid(d){
    t = std::thread(&Worker::run, this);
}

Worker::~Worker() {
    t.join();
}