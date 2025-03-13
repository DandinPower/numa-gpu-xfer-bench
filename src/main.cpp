#include "common.h"
#include "memory.h"



int main(int argc, char* argv[]) {
    // Argument parsing with flags
    int ngpus = -1;
    size_t benchmark_bytes = 0;
    int iterations_per_bench = 0;
    std::string operation_type;
    int pin_memory = -1;

    for (int i = 1; i < argc; ) {
        std::string arg = argv[i];
        if (arg == "--ngpus") {
            if (i + 1 < argc) {
                ngpus = std::stoi(argv[i + 1]);
                i += 2;
            } else {
                std::cerr << "Missing value for --ngpus" << std::endl;
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
        } else if (arg == "--operation_type") {
            if (i + 1 < argc) {
                operation_type = argv[i + 1];
                i += 2;
            } else {
                std::cerr << "Missing value for --operation_type" << std::endl;
                return 1;
            }
        } else if (arg == "--pin_memory") {
            if (i + 1 < argc) {
                pin_memory = std::stoi(argv[i + 1]);
                i += 2;
            } else {
                std::cerr << "Missing value for --pin_memory" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return 1;
        }
    }

    // Validate all required arguments
    if (ngpus < 0 || benchmark_bytes == 0 || iterations_per_bench <= 0 || 
        operation_type.empty() || pin_memory < 0) {
        std::cerr << "Usage: " << argv[0] << " --ngpus <int> --benchmark_bytes <size_t> "
                  << "--iterations_per_bench <int> --operation_type <string> "
                  << "--pin_memory <0 or 1>" << std::endl;
        return 1;
    }

    bool pin_memory_bool = (pin_memory != 0); // 0 means no pinning, non-zero means pin

    // Parse operation type
    auto ops = split(operation_type, ',');
    bool do_read = std::find(ops.begin(), ops.end(), "R") != ops.end();
    bool do_write = std::find(ops.begin(), ops.end(), "W") != ops.end();
    if (!do_read && !do_write) {
        std::cerr << "Invalid operation_type: must include 'R' and/or 'W'" << std::endl;
        return 1;
    }

    
    size_t aligned_benchmark_bytes = ((benchmark_bytes + DMA_ALIGNMENT - 1) / DMA_ALIGNMENT) * DMA_ALIGNMENT;

    std::vector<void*> gpu_buffer_R(ngpus, nullptr);
    std::vector<void*> gpu_buffer_W(ngpus, nullptr);
    std::vector<cudaStream_t> stream_R(ngpus);
    std::vector<cudaStream_t> stream_W(ngpus);
    std::vector<cudaEvent_t> start_R(ngpus);
    std::vector<cudaEvent_t> end_R(ngpus);
    std::vector<cudaEvent_t> start_W(ngpus);
    std::vector<cudaEvent_t> end_W(ngpus);
    std::vector<void*> cpu_buffer_R;
    std::vector<void*> cpu_buffer_W;
    std::vector<float> host_to_device_times;
    std::vector<float> device_to_host_times;
    if (do_read) {
        for (int gpu = 0; gpu < ngpus; ++gpu) {
            CUDA_CHECK(cudaSetDevice(gpu));
            CUDA_CHECK(cudaMalloc(&gpu_buffer_R[gpu], aligned_benchmark_bytes));
            CUDA_CHECK(cudaStreamCreate(&stream_R[gpu]));
            CUDA_CHECK(cudaEventCreate(&start_R[gpu]));
            CUDA_CHECK(cudaEventCreate(&end_R[gpu]));
        }

        cpu_buffer_R.resize(ngpus);
        for (int gpu = 0; gpu < ngpus; ++gpu) {
            cpu_buffer_R[gpu] = allocate_cpu_buffer(aligned_benchmark_bytes, pin_memory_bool);
        }
        for (int iter = 0; iter < iterations_per_bench; ++iter) {
            for (int gpu = 0; gpu < ngpus; ++gpu) {
                CUDA_CHECK(cudaSetDevice(gpu));
                CUDA_CHECK(cudaEventRecord(start_R[gpu], stream_R[gpu]));
                CUDA_CHECK(cudaMemcpyAsync(gpu_buffer_R[gpu], cpu_buffer_R[gpu], aligned_benchmark_bytes, cudaMemcpyHostToDevice, stream_R[gpu]));
                CUDA_CHECK(cudaEventRecord(end_R[gpu], stream_R[gpu]));
            }
            for (int gpu = 0; gpu < ngpus; ++gpu) {
                CUDA_CHECK(cudaSetDevice(gpu));
                CUDA_CHECK(cudaDeviceSynchronize());
            }
            for (int gpu = 0; gpu < ngpus; ++gpu) {
                float time_R;
                CUDA_CHECK(cudaEventElapsedTime(&time_R, start_R[gpu], end_R[gpu]));
                host_to_device_times.push_back(time_R);
            }
        }

        float sum_time_R = 0;
        float sum_bandwidth_R = 0;
        for (auto time : host_to_device_times) {
            sum_time_R += time;
            sum_bandwidth_R += static_cast<double>(aligned_benchmark_bytes) / (time / 1000.0);
        }
        int num_transfers_R = host_to_device_times.size();
        float avg_latency_R = sum_time_R / num_transfers_R;
        float avg_bandwidth_R = (sum_bandwidth_R / num_transfers_R) / (1024*1024*1024);
        std::cout << "Average host-to-device latency: " << avg_latency_R << " ms" << std::endl;
        std::cout << "Average host-to-device bandwidth: " << avg_bandwidth_R << " GiB/s" << std::endl;

        for (int gpu = 0; gpu < ngpus; ++gpu) {
            CUDA_CHECK(cudaSetDevice(gpu));
            CUDA_CHECK(cudaFree(gpu_buffer_R[gpu]));
            CUDA_CHECK(cudaStreamDestroy(stream_R[gpu]));
            CUDA_CHECK(cudaEventDestroy(start_R[gpu]));
            CUDA_CHECK(cudaEventDestroy(end_R[gpu]));
        }

        for (auto ptr : cpu_buffer_R) {
            free_cpu_buffer(ptr, pin_memory_bool);
        }
    }

    if (do_write) {
        for (int gpu = 0; gpu < ngpus; ++gpu) {
            CUDA_CHECK(cudaSetDevice(gpu));
            CUDA_CHECK(cudaMalloc(&gpu_buffer_W[gpu], aligned_benchmark_bytes));
            CUDA_CHECK(cudaStreamCreate(&stream_W[gpu]));
            CUDA_CHECK(cudaEventCreate(&start_W[gpu]));
            CUDA_CHECK(cudaEventCreate(&end_W[gpu]));
        }

        cpu_buffer_W.resize(ngpus);
        for (int gpu = 0; gpu < ngpus; ++gpu) {
            cpu_buffer_W[gpu] = allocate_cpu_buffer(aligned_benchmark_bytes, pin_memory_bool);
        }
        for (int iter = 0; iter < iterations_per_bench; ++iter) {
            for (int gpu = 0; gpu < ngpus; ++gpu) {
                CUDA_CHECK(cudaSetDevice(gpu));
                CUDA_CHECK(cudaEventRecord(start_W[gpu], stream_W[gpu]));
                CUDA_CHECK(cudaMemcpyAsync(cpu_buffer_W[gpu], gpu_buffer_W[gpu], aligned_benchmark_bytes, cudaMemcpyDeviceToHost, stream_W[gpu]));
                CUDA_CHECK(cudaEventRecord(end_W[gpu], stream_W[gpu]));
            }
            for (int gpu = 0; gpu < ngpus; ++gpu) {
                CUDA_CHECK(cudaSetDevice(gpu));
                CUDA_CHECK(cudaDeviceSynchronize());
            }
            for (int gpu = 0; gpu < ngpus; ++gpu) {
                float time_W;
                CUDA_CHECK(cudaEventElapsedTime(&time_W, start_W[gpu], end_W[gpu]));
                device_to_host_times.push_back(time_W);
            }
        }

        float sum_time_W = 0;
        float sum_bandwidth_W = 0;
        for (auto time : device_to_host_times) {
            sum_time_W += time;
            sum_bandwidth_W += static_cast<double>(aligned_benchmark_bytes) / (time / 1000.0);
        }
        int num_transfers_W = device_to_host_times.size();
        float avg_latency_W = sum_time_W / num_transfers_W;
        float avg_bandwidth_W = (sum_bandwidth_W / num_transfers_W) / (1024*1024*1024);
        std::cout << "Average device-to-host latency: " << avg_latency_W << " ms" << std::endl;
        std::cout << "Average device-to-host bandwidth: " << avg_bandwidth_W << " GiB/s" << std::endl;
    
        for (int gpu = 0; gpu < ngpus; ++gpu) {
            CUDA_CHECK(cudaSetDevice(gpu));
            CUDA_CHECK(cudaFree(gpu_buffer_W[gpu]));
            CUDA_CHECK(cudaStreamDestroy(stream_W[gpu]));
            CUDA_CHECK(cudaEventDestroy(start_W[gpu]));
            CUDA_CHECK(cudaEventDestroy(end_W[gpu]));
        }

        for (auto ptr : cpu_buffer_W) {
            free_cpu_buffer(ptr, pin_memory_bool);
        }
    }
    
    return 0;
}