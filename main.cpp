#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <queue>
#include <string>
#include <cmath>
#include <ctime>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include "parallel_util.h"

#define NUM_THREADS 4
#define MAX_QUEUE_SIZE 100
int ROWS_PER_TASK = N;


using namespace std;

void saveMatrixToCSV(const string& filename, double** matrix, int rows, int cols) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << " for writing.\n";
        return;
    }
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            file << matrix[i][j];
            if (j < cols - 1) file << ",";
        }
        file << "\n";
    }
    file.close();
    cout << "Matrix saved to " << filename << "\n";
}

double calculateU(double Z) {
    if (Z <= 0) return -1.0;
    return -0.21436 + 1.35034 * log10(Z);
}

double calculatePso(double Z) {
    double U = calculateU(Z);
    double u_pow[9];
    u_pow[0] = 1.0;
    for (int i = 1; i < 9; i++) {
        u_pow[i] = u_pow[i - 1] * U;
    }
    double log_pso = 0.0;
    for (int i = 0; i < 9; i++) {
        log_pso += c[i] * u_pow[i];
    }
    return pow(10, log_pso);
}

double** createGrid() {
    double** grid = new double*[N];
    for (int i = 0; i < N; i++) {
        grid[i] = new double[N]();
    }
    return grid;
}

double** createDistGrid() {
    double** dist_grid = new double*[N];
    for (int i = 0; i < N; i++) {
        dist_grid[i] = new double[N]();
        for (int j = 0; j < N; j++) {
            dist_grid[i][j] = sqrt(pow((i - middle) * cell_size, 2) + pow((j - middle) * cell_size, 2));
        }
    }
    return dist_grid;
}

void deleteGrid(double** grid) {
    for (int i = 0; i < N; i++) {
        delete[] grid[i];
    }
    delete[] grid;
}

double sequential_sim(double** grid, double** dist_grid) {
    clock_t start = clock();
    for (int t = 1; t <= max_t; t++) {
        int max_R = t * 343;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (dist_grid[i][j] <= max_R && grid[i][j] == 0.0) {
                    double Z = dist_grid[i][j] / pow(W, 1.0 / 3.0);
                    grid[i][j] = calculatePso(Z);
                }
            }
        }
    }
    clock_t end = clock();
    return static_cast<double>(end - start) / CLOCKS_PER_SEC;
}

double parallel_sim(double** grid, double** dist_grid) {
    TaskQueue queue;
    vector<Worker*> workers;
    // Start worker threads
    for (int i = 0; i < NUM_THREADS; i++) {
        workers.push_back(new Worker(i, queue, grid, dist_grid));
    }
    double previous_R = -1.0;
    clock_t start = clock();
    // Enqueue tasks for each time step
    for (int t = 1; t <= max_t; t++) {
        for (int i = 0; i < N; i += ROWS_PER_TASK) {
            double max_R = t * 343;
            int end_row = min(i + ROWS_PER_TASK - 1, N - 1); // Inclusive end
            queue.enqueue(new Task(i, end_row, t, max_R, previous_R));
            previous_R = max_R;

        }
    }
    // Ensure all tasks are done before shutdown
    queue.setShutdown();
    for (Worker* worker : workers) {
        delete worker;
    }

    clock_t end = clock();
    return static_cast<double>(end - start) / CLOCKS_PER_SEC;
}

bool isEqualGrid(double** grid1, double** grid2) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (abs(grid1[i][j] - grid2[i][j]) > 1e-10) return false;
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
    else cout << "Parallel simulation is incorrect" << endl;
    cout << "Sequential time: " << seq_time << " seconds\n";
    cout << "Parallel time: " << par_time << " seconds\n";
    cout << "Speedup: " << seq_time / par_time << "x\n";

    // saveMatrixToCSV("seq.csv", grid_seq, N, N);
    // saveMatrixToCSV("par.csv", grid_par, N, N);

    deleteGrid(grid_seq);
    deleteGrid(grid_par);
    deleteGrid(dist_grid);

    return 0;
}