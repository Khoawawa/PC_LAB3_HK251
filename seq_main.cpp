#include <iostream>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include <cmath>
#include <atomic>
#include "parallel_util.h"
using namespace std;
#define NUM_THREADS 4
#define MAX_QUEUE_SIZE 100
int N = 4000;
int cell_size = 10;
int max_t = 100;
int middle = N / 2;
double W = 5000 * 1e6;

double c[] = {
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

using namespace std;
double calculateU(double Z){
    return -0.21436 + 1.35034 * log10(Z);
}
double calculatePso(double Z){
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
double ** createGrid(){
    double** grid = new double*[N];
    for (int i = 0; i < N; i++){
        grid[i] = new double[N]();
        for (int j = 0; j < N; j++){
            grid[i][j] = 0;
        }
    }
    return grid;
}
double ** createDistGrid(){
    double** dist_grid = new double*[N];
    for (int i = 0; i < N; i++){
        dist_grid[i] = new double[N]();
        for (int j = 0; j < N; j++){
            dist_grid[i][j] = sqrt(pow((i - middle) * cell_size, 2) + pow((j - middle) * cell_size, 2));
        }
    }
    return dist_grid;
}
void deleteGrid(double** grid){
    for (int i = 0; i < N; i++){
        delete[] grid[i];
    }
    delete[] grid;
}
double parallel_sim(double** grid, double** dist_grid){
    clock_t start = clock();
    TaskQueue queue;
    atomic<int> tasks_complete(0);
    vector<Worker*> workers;
    const int total_tasks = N * max_t;
    for (int i = 0 ; i < NUM_THREADS; i++){
        workers.push_back(new Worker(i, queue, grid, dist_grid,tasks_complete));
    }

    for (int t = 1; t <= max_t; t++){
        for(int i = 0; i < N; i++){
            queue.enqueue(new Task(i, t));
        }
    }
    //TODO: implement wait for all threads to finish
    while(tasks_complete < total_tasks){
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    queue.setShutdown();
    for(Worker* worker : workers){
        delete worker;
    }
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}
double sequential_sim(double** grid, double** dist_grid){
    clock_t start = clock();
    for (int t = 1 ; t <= max_t; t++){
        // time stepping
        int max_R = t * 343;
        for(int i = 0; i < N; i++){
            for(int j = 0; j < N; j++){
                if (dist_grid[i][j] <= max_R && grid[i][j] == 0){
                    double Z = dist_grid[i][j] / pow(W, 1.0 / 3.0);
                    grid[i][j] = calculatePso(Z);                
                }
            }
        }
    }
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}
bool isEqualGrid(double** grid1, double** grid2){
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            if (grid1[i][j] != grid2[i][j]) return false;
        }
    }
    return true;
}
int main() {
    double** grid_seq = createGrid();
    double** grid_par = createGrid();
    double** dist_grid = createDistGrid();

    double seq_time = sequential_sim(grid_seq, dist_grid);
    double par_time = parallel_sim(grid_par, dist_grid);
    if (isEqualGrid(grid_seq, grid_par)) cout << "Parallel simulation is correct" << endl;
    cout << "Sequential time: " << seq_time << endl;
    cout << "Parallel time: " << par_time << endl;
    cout << "Speedup: " << seq_time / par_time << endl;
    deleteGrid(grid_seq);
    deleteGrid(grid_par);
    deleteGrid(dist_grid);
    return 0;
}