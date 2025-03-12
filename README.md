# numa-gpu-xfer-bench

This NUMA-aware benchmark provides latency and bandwidth measurements for CPU-GPU data transfers using libnuma for interleaved allocation, cudaMemcpyAsync for async operations, and concurrent multi-GPU support via command-line options.

## Prerequisites

To build and run this benchmark, ensure the following requirements are met:

- **System**: A machine with multiple NUMA nodes and multiple NVIDIA GPUs.
- **CUDA Toolkit**: Installed and configured (e.g., `/usr/local/cuda`). Ensure `LD_LIBRARY_PATH` includes `/usr/local/cuda/lib64`.
- **libnuma**: Development package installed for NUMA-aware memory allocation.

On Ubuntu, install libnuma with:

```bash
sudo apt-get install libnuma-dev
```

Verify CUDA installation by checking `nvcc --version`. If not installed, download and install it from the [NVIDIA CUDA Toolkit page](https://developer.nvidia.com/cuda-toolkit).

## Building

Clone the repository and build the benchmark:

```bash
git clone <repository-url>
cd numa-gpu-xfer-bench
make
```

This generates the `benchmark` executable in the `/build` directory.

## Usage

Run the benchmark with the following command-line arguments:

```bash
./build/benchmark --ngpus <int> --benchmark_bytes <size_t> --iterations_per_bench <int> --operation_type <string> --numa_nodes <string> --pin_memory <0 or 1>
```

### Arguments
- `--ngpus`: Number of GPUs to use (e.g., `2`).
- `--benchmark_bytes`: Size of data to transfer in bytes (e.g., `1048576` for 1 MB).
- `--iterations_per_bench`: Number of iterations per benchmark (e.g., `100`).
- `--operation_type`: Operations to perform (`R` for host-to-device, `W` for device-to-host, `R,W` for both).
- `--numa_nodes`: Comma-separated NUMA nodes for interleaved allocation (e.g., `"0,1"`).
- `--pin_memory`: Pin CPU memory for CUDA (`0` to disable, `1` to enable).

### Example
```bash
./build/benchmark --ngpus 2 --benchmark_bytes 1048576 --iterations_per_bench 100 --operation_type "R,W" --numa_nodes "0,1" --pin_memory 1
```

This runs the benchmark on 2 GPUs, transferring 1 MB of data 100 times, performing both read and write operations, using NUMA nodes 0 and 1, with pinned memory.

## Output
The program outputs average latency (in ms) and bandwidth (in GB/s) for host-to-device and/or device-to-host transfers, depending on the `operation_type`.

## Notes
- Ensure the number of GPUs specified not exceeded the system's available GPUs.
- When the number of GPUs is greater than two, remember that they will execute benchmarks simultaneously and asynchronously, causing the operations to overlap.
- NUMA node indices should be valid for your system (check with `numactl --hardware`).
- Pinning memory (`--pin_memory 1`) may improve performance but increases setup overhead.