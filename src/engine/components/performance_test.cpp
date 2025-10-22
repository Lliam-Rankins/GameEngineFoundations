#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <fstream> // For writing to CSV
#include <numeric> // For std::accumulate
#include <cmath>   // For std::sqrt
#include <cstdlib> // For system()
#include <csignal> // For process signals

// Include the server's header so we can call its functions
#include "../headers/server_testable.h"

// --- Client Spawning (Simple Version) ---
// This uses `system()` to launch clients in new terminals.
// This is simple but may not be the most robust.
// You'll need to update "path/to/your/client/executable"
//
// For Windows:
// const std::string CLIENT_EXE = "start \"Client\" \"path\\to\\client.exe\"";
//
// For Linux/macOS:
// const std::string CLIENT_EXE = "xterm -e \"path/to/client/executable\" &";
//
// Let's assume Linux/macOS for this example:
const std::string CLIENT_EXE = "../build/client &"; // CHANGE THIS

/**
 * Spawns N client processes.
 * This is a basic implementation.
 */
void spawn_clients(int num_clients)
{
    std::cout << "  Spawning " << num_clients << " clients..." << std::endl;
    for (int i = 0; i < num_clients; ++i)
    {
        system(CLIENT_EXE.c_str());
        // Give them a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // Give all clients time to connect
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

/**
 * Kills all client processes.
 * This is a 'brute force' way.
 */
void stop_clients() {
    std::cout << "  Stopping clients..." << std::endl;
    
    // We will try three different pkill commands to be thorough.
    // 1. Try to kill by exact executable name
    int res1 = system("pkill -x client");
    
    // 2. Try to kill by the common WSL process name (if it's a .exe)
    int res2 = system("pkill -f client.exe");
    
    // 3. Try the original broad command
    int res3 = system("pkill -f client");

    // Use -9 (force kill) on any remaining processes
    system("pkill -9 -f client");
    system("pkill -9 -x client");

    std::cout << "  Client kill commands sent. Pausing 2s for cleanup." << std::endl;
    // Give the OS time to kill them all before the next run
    std::this_thread::sleep_for(std::chrono::seconds(2)); 
}

int main()
{
    const long NUM_ITERATIONS = 100000;
    const int NUM_RUNS = 5; // Run each experiment 5 times

    // --- This is your Experiment Design ---
    std::vector<std::string> strategies = {"FullState", "DeltaState"};
    std::vector<int> client_counts = {1, 5, 10};
    std::vector<int> static_object_counts = {10}; // Per prompt, we vary moving objects
    std::vector<int> moving_object_counts = {10, 100, 500};

    std::ofstream results_file;
    results_file.open("performance_results.csv");
    results_file << "Strategy,NumClients,StaticObjects,MovingObjects,Run,TimeTaken_ms\n";

    for (const auto &strategy : strategies)
    {
        for (int num_clients : client_counts)
        {
            for (int num_static : static_object_counts)
            {
                for (int num_moving : moving_object_counts)
                {

                    std::cout << "-------------------------------------------\n";
                    std::cout << "STARTING TEST:\n"
                              << "  Strategy: " << strategy << "\n"
                              << "  Clients:  " << num_clients << "\n"
                              << "  Static:   " << num_static << "\n"
                              << "  Moving:   " << num_moving << "\n"
                              << "-------------------------------------------\n";

                    std::vector<double> run_times;
                    for (int run = 1; run <= NUM_RUNS; ++run)
                    {
                        std::cout << "  Starting Run " << run << "/" << NUM_RUNS << "...\n";

                        // 1. START THE SERVER IN A SEPARATE THREAD
                        auto start_time = std::chrono::high_resolution_clock::now();
                        std::thread server_thread(run_server_experiment,
                                                  NUM_ITERATIONS,
                                                  strategy,
                                                  num_static,
                                                  num_moving,
                                                  num_clients // Pass the client count
                        );

                        // 2. Wait a moment for the server to start its threads and bind ports
                        //    The server will print "[Server] Waiting for 1 clients..."
                        std::this_thread::sleep_for(std::chrono::seconds(1));

                        // 3. NOW SPAWN THE CLIENTS
                        spawn_clients(num_clients);

                        // 4. Wait for the server thread to finish its work
                        //    (It will unblock once all clients connect, then run its 100k loops)
                        std::cout << "  Test is running... waiting for server to finish." << std::endl;
                        server_thread.join(); // This blocks until run_server_experiment returns
                        auto end_time = std::chrono::high_resolution_clock::now();

                        // 5. Stop the clients
                        stop_clients();

                        // --- END CORRECT ORDER ---

                        // 6. Record data
                        std::chrono::duration<double, std::milli> time_taken_ms = end_time - start_time;
                        double duration = time_taken_ms.count();

                        run_times.push_back(duration);
                        results_file << strategy << "," << num_clients << ","
                                     << num_static << "," << num_moving << ","
                                     << run << "," << duration << "\n";

                        std::cout << "  Run " << run << " complete. Time: " << duration << " ms\n";
                    }

                    // --- Calculate Avg and Variance for this test set ---
                    double sum = std::accumulate(run_times.begin(), run_times.end(), 0.0);
                    double mean = sum / run_times.size();

                    double sq_sum = 0.0;
                    for (const double &d : run_times)
                    {
                        sq_sum += (d - mean) * (d - mean);
                    }
                    double variance = (run_times.size() > 1) ? sq_sum / (run_times.size() - 1) : 0.0;

                    std::cout << "\n  TEST SET COMPLETE\n"
                              << "  Avg: " << mean << " ms\n"
                              << "  Variance: " << variance << "\n"
                              << "-------------------------------------------\n\n";
                }
            }
        }
    }

    results_file.close();
    std::cout << "All experiments complete. Results saved to performance_results.csv\n";
    return 0;
}