#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <chrono>     // For high-resolution timing
#include <numeric>    // For std::accumulate
#include <iomanip>    // For std::setprecision
#include <cstring>    // For memcpy and memset
#include <numa.h>     // For NUMA-aware allocation
#include <omp.h>      // For OpenMP

int main(int argc, char* argv[]) {
    // --- Argument Parsing ---
    std::string src_nodes_str;
    std::string dst_nodes_str;
    size_t benchmark_bytes = 0;
    int iterations_per_bench = 0;
    int num_threads = 1;

    for (int i = 1; i < argc; ) {
        std::string arg = argv[i];
        if (arg == "--src_nodes") {
            if (i + 1 < argc) {
                src_nodes_str = argv[i + 1];
                i += 2;
            } else {
                std::cerr << "Missing value for --src_nodes" << std::endl;
                return 1;
            }
        } else if (arg == "--dst_nodes") {
            if (i + 1 < argc) {
                dst_nodes_str = argv[i + 1];
                i += 2;
            } else {
                std::cerr << "Missing value for --dst_nodes" << std::endl;
                return 1;
            }
        } else if (arg == "--benchmark_bytes") {
            if (i + 1 < argc) {
                benchmark_bytes = std::stoul(argv[i + 1]);
                i += 2;
            } else {
                std::cerr << "Missing value for --benchmark_bytes" << std::endl;
                return 1;
            }
        } else if (arg == "--iterations_per_bench") {
            if (i + 1 < argc) {
                iterations_per_bench = std::stoi(argv[i + 1]);
                i += 2;
            } else {
                std::cerr << "Missing value for --iterations_per_bench" << std::endl;
                return 1;
            }
        } else if (arg == "--num_threads") {
            if (i + 1 < argc) {
                num_threads = std::stoi(argv[i + 1]);
                i += 2;
            } else {
                std::cerr << "Missing value for --num_threads" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 1;
        }
    }

    // --- Validate all required arguments ---
    if (src_nodes_str.empty() || dst_nodes_str.empty() || benchmark_bytes == 0 || iterations_per_bench <= 0 || num_threads <= 0) {
        std::cerr << "Usage: " << argv[0] 
                  << " --src_nodes <\"0,1\" or \"0-3\"> --dst_nodes <\"0,1\" or \"0-3\"> "
                  << "--benchmark_bytes <size_t> "
                  << "--iterations_per_bench <int> --num_threads <int>" << std::endl;
        return 1;
    }

    // --- NUMA Sanity Check ---
    if (numa_available() == -1) {
        std::cerr << "Error: NUMA support is not available on this system." << std::endl;
        return 1;
    }

    // --- Parse Node Strings into Bitmasks ---
    struct bitmask *src_mask = numa_parse_nodestring(src_nodes_str.c_str());
    if (!src_mask) {
        std::cerr << "Error: Invalid source nodestring: " << src_nodes_str << std::endl;
        return 1;
    }

    struct bitmask *dst_mask = numa_parse_nodestring(dst_nodes_str.c_str());
    if (!dst_mask) {
        std::cerr << "Error: Invalid destination nodestring: " << dst_nodes_str << std::endl;
        numa_bitmask_free(src_mask); // Clean up
        return 1;
    }

    std::cout << "--- Configuration ---" << std::endl;
    std::cout << "Source Nodes: " << src_nodes_str << std::endl;
    std::cout << "Destination Nodes: " << dst_nodes_str << std::endl;
    std::cout << "Buffer Size: " << static_cast<double>(benchmark_bytes) / (1024*1024) << " MiB" << std::endl;
    std::cout << "Iterations: " << iterations_per_bench << std::endl;
    std::cout << "OpenMP Threads: " << num_threads << std::endl;
    std::cout << "---------------------" << std::endl;


    // --- Allocate NUMA-aware interleaved memory ---
    void* src_buffer = numa_alloc_interleaved_subset(benchmark_bytes, src_mask);
    if (!src_buffer) {
        std::cerr << "Error: Failed to allocate " << benchmark_bytes << " bytes on NUMA nodes " << src_nodes_str << std::endl;
        numa_bitmask_free(src_mask);
        numa_bitmask_free(dst_mask);
        return 1;
    }

    void* dst_buffer = numa_alloc_interleaved_subset(benchmark_bytes, dst_mask);
    if (!dst_buffer) {
        std::cerr << "Error: Failed to allocate " << benchmark_bytes << " bytes on NUMA nodes " << dst_nodes_str << std::endl;
        numa_free(src_buffer, benchmark_bytes); // Clean up
        numa_bitmask_free(src_mask);
        numa_bitmask_free(dst_mask);
        return 1;
    }

    // --- Initialize buffers in parallel to fault pages ---
    // This is still critical. It forces the OS to apply the interleaving
    // policy and physically allocate the pages before we time the copy.
    #pragma omp parallel num_threads(num_threads)
    {
        int thread_id = omp_get_thread_num();
        int total_threads = omp_get_num_threads();
        size_t chunk_size = benchmark_bytes / total_threads;
        size_t offset = thread_id * chunk_size;

        if (thread_id == total_threads - 1) {
            chunk_size = benchmark_bytes - offset;
        }

        memset(static_cast<char*>(src_buffer) + offset, 0, chunk_size);
        memset(static_cast<char*>(dst_buffer) + offset, 1, chunk_size);
    }

    // --- Run Benchmark ---
    std::vector<double> timings_ms;
    for (int iter = 0; iter < iterations_per_bench; ++iter) {
        
        auto start = std::chrono::high_resolution_clock::now();

        #pragma omp parallel num_threads(num_threads)
        {
            int thread_id = omp_get_thread_num();
            int total_threads = omp_get_num_threads();
            size_t chunk_size = benchmark_bytes / total_threads;
            size_t offset = thread_id * chunk_size;

            if (thread_id == total_threads - 1) {
                chunk_size = benchmark_bytes - offset;
            }

            memcpy(static_cast<char*>(dst_buffer) + offset,
                   static_cast<char*>(src_buffer) + offset,
                   chunk_size);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed_ms = end - start;
        timings_ms.push_back(elapsed_ms.count());
    }

    // --- Report Results ---
    double sum_time_ms = std::accumulate(timings_ms.begin(), timings_ms.end(), 0.0);
    double avg_latency_ms = sum_time_ms / timings_ms.size();
    
    double avg_latency_s = avg_latency_ms / 1000.0;
    double total_bytes_per_sec = static_cast<double>(benchmark_bytes) / avg_latency_s;
    double avg_bandwidth_gib_s = total_bytes_per_sec / (1024.0 * 1024.0 * 1024.0);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "--- Results ---" << std::endl;
    std::cout << "Average Latency: " << avg_latency_ms << " ms" << std::endl;
    std::cout << "Average Bandwidth: " << avg_bandwidth_gib_s << " GiB/s" << std::endl;
    std::cout << "---------------" << std::endl;

    // --- Cleanup ---
    if (src_mask) {
        numa_bitmask_free(src_mask);
    }
    if (dst_mask) {
        numa_bitmask_free(dst_mask);
    }

    if (src_buffer) {
        numa_free(src_buffer, benchmark_bytes);
    }
    if (dst_buffer) {
        numa_free(dst_buffer, benchmark_bytes);
    }

    return 0;
}