# numa-gpu-xfer-bench

This benchmark provides latency and bandwidth measurements for CPU-GPU data transfers, utilizing `posix_memalign` for memory allocation and `numactl` for controlling NUMA memory policies. It supports asynchronous operations with `cudaMemcpyAsync` and concurrent multi-GPU execution via command-line options.

## Prerequisites

To build and run this benchmark, ensure the following requirements are met:

- **System**: A machine with multiple NUMA nodes and multiple NVIDIA GPUs.
- **CUDA Toolkit**: Installed and configured (e.g., `/usr/local/cuda`).
- **numactl**: Installed to control NUMA memory policies.

On Ubuntu, install `numactl` with:

```bash
sudo apt-get install numactl
```

Verify CUDA installation by checking `nvcc --version`. If not installed, download and install it from the [NVIDIA CUDA Toolkit page](https://developer.nvidia.com/cuda-toolkit).

### Optional: Weighted Interleave

The weighted interleave feature requires additional setup:

1. **Kernel Requirement**: Linux kernel ≥ 6.9 (me tested on 6.12.18) to support `/sys/kernel/mm/mempolicy/weighted_interleave/node*`. Check your kernel version with:
   ```bash
   uname -r
   ```
   Update your kernel if necessary.

2. **Latest `numactl`**: Weighted interleave requires `numactl` ≥ 2.0.19 (APT provides 2.0.18), so compile it manually:
   ```bash
   sudo apt install autoconf automake libtool # tools required to compile numactl
   sudo apt remove numactl libnuma-dev
   git clone https://github.com/numactl/numactl.git
   cd numactl
   git checkout v2.0.19
   ./autogen.sh
   ./configure
   make
   make test
   sudo make install
   sudo ldconfig
   ```
   Verify the version with:
   ```bash
   numactl --version
   ```

**Note**: The weighted interleave feature is optional. If you do not plan to use it, the standard `numactl` package and an older kernel version suffice.

## Building

Clone the repository and build the benchmark:

```bash
git clone <repository-url>
cd numa-gpu-xfer-bench
make
```

This generates the `benchmark` executable in the `build/` directory.

## Usage

Run the benchmark with `numactl` to set the desired NUMA memory policy. The general command is:

```bash
numactl [numactl options] ./build/benchmark --ngpus <int> --benchmark_bytes <size_t> --iterations_per_bench <int> --operation_type <string> --pin_memory <0 or 1>
```

### Examples

- **Interleave memory allocation across NUMA nodes 0 and 1:**
  ```bash
  numactl --interleave=0,1 ./build/benchmark --ngpus 2 --benchmark_bytes 1048576 --iterations_per_bench 100 --operation_type R,W --pin_memory 1
  ```

- **Bind memory allocation to NUMA node 0:**
  ```bash
  numactl --interleave=0 ./build/benchmark --ngpus 2 --benchmark_bytes 1048576 --iterations_per_bench 100 --operation_type R,W --pin_memory 1
  ```

- **Weighted interleave across NUMA nodes 0 and 3 with weights 1 and 2:**
  First, set the weights (requires root access):
  ```bash
  echo 1 > /sys/kernel/mm/mempolicy/weighted_interleave/node0
  echo 2 > /sys/kernel/mm/mempolicy/weighted_interleave/node3
  ```
  Then, run the benchmark:
  ```bash
  numactl --weighted-interleave=0,3 ./build/benchmark --ngpus 2 --benchmark_bytes 1048576 --iterations_per_bench 100 --operation_type R,W --pin_memory 1
  ```

### Arguments

- `--ngpus`: Number of GPUs to use (e.g., `2`).
- `--benchmark_bytes`: Size of data to transfer in bytes (e.g., `1048576` for 1 MB).
- `--iterations_per_bench`: Number of iterations per benchmark (e.g., `100`).
- `--operation_type`: Operations to perform (`R` for host-to-device, `W` for device-to-host, `R,W` for both).
- `--pin_memory`: Pin CPU memory for CUDA (`0` to disable, `1` to enable).

## Output

The program outputs average latency (in milliseconds) and bandwidth (in gigibytes 2^30 bytes per second, GiB/s) for host-to-device and/or device-to-host transfers, depending on the `operation_type`. For example:

```
Average host-to-device latency: 0.038 ms
Average host-to-device bandwidth: 55.58 GiB/s
Average device-to-host latency: 0.039 ms
Average device-to-host bandwidth: 55.50 GiB/s
```

## Benchmark Scripts

The repository includes two Bash scripts to automate running the benchmark and generate CSV-formatted results.

### Standard Interleave Script

The `interleave_scripts.sh` script uses `numactl --interleave` to test various configurations:

```bash
bash interleave_scripts.sh > example/one_cxl_aic_with_interleave.csv
```

This redirects output to `example/one_cxl_aic_with_interleave.csv`. The script assumes the `benchmark` executable is in `build/`.

#### Script Details

- **Configurations Tested**:
  - Number of GPUs (`ngpus`): `1`, `2`
  - NUMA node combinations (`numa_nodes`): `"0"`, `"0,1"`, `"3"`, `"0,3"` (node 3 is a CXL memory node in the example system)
  - Data transfer sizes (`benchmark_bytes`): 1 KB (`1024`) to 2 GB (`2147483648`), in powers of 2
- **Fixed Parameters**:
  - Iterations per benchmark: `100`
  - Operation type: `"R,W"` (both read and write)
  - Memory pinning: Enabled (`--pin_memory 1`)
- **Output Format**:
  CSV columns:
  - `ngpus`: Number of GPUs used
  - `numa_nodes`: Comma-separated list of NUMA nodes
  - `access_size(bytes)`: Size of data transferred
  - `read_latency(ms)`: Average host-to-device latency
  - `read_bw(gib/s)`: Average host-to-device bandwidth (in GiB/s)
  - `write_latency(ms)`: Average device-to-host latency
  - `write_bw(gib/s)`: Average device-to-host bandwidth (in GiB/s)

#### Customization

Modify `interleave_scripts.sh` to adjust:
- `ngpus_items`: Number of GPUs
- `numa_nodes_items`: NUMA topology (check with `numactl --hardware`)
- `benchmark_bytes_items`: Transfer sizes

### Weighted Interleave Script

The `weighted_interleave_scripts.sh` script uses `numactl --weighted-interleave` to test weighted configurations:

```bash
bash weighted_interleave_scripts.sh > example/one_cxl_aic_with_weighted_interleave.csv
```

This redirects output to `example/one_cxl_aic_with_weighted_interleave.csv`. The script sets weights via `/sys/kernel/mm/mempolicy/weighted_interleave/` and assumes the `benchmark` executable is in `build/`.

#### Script Details

- **Configurations Tested**:
  - Number of GPUs (`ngpus`): `1`, `2`
  - NUMA node combination (`numa_nodes`): `"0,3"` (node 0 is local CPU, node 3 is CXL memory in the example)
  - Weight configurations (`numa_weight`): `"1,1"`, `"1,2"`, `"1,3"`, `"1,4"` (weights for nodes 0 and 3)
  - Data transfer sizes (`benchmark_bytes`): 4 KB (`4096`) to 2 GB (`2147483648`), in powers of 2
- **Fixed Parameters**:
  - Iterations per benchmark: `100`
  - Operation type: `"R,W"` (both read and write)
  - Memory pinning: Enabled (`--pin_memory 1`)
- **Output Format**:
  CSV columns:
  - `ngpus`: Number of GPUs used
  - `numa_nodes`: Comma-separated list of NUMA nodes
  - `numa_weight`: Comma-separated weights for the nodes
  - `access_size(bytes)`: Size of data transferred
  - `read_latency(ms)`: Average host-to-device latency
  - `read_bw(gib/s)`: Average host-to-device bandwidth (in GiB/s)
  - `write_latency(ms)`: Average device-to-host latency
  - `write_bw(gib/s)`: Average device-to-host bandwidth (in GiB/s)

#### Customization

Modify `weighted_interleave_scripts.sh` to adjust:
- `ngpus_items`: Number of GPUs
- `LOCAL_NODE` and `CXL_NODE`: NUMA nodes (check with `numactl --hardware`)
- `numa_weight_items`: Weight configurations
- `benchmark_bytes_items`: Transfer sizes

**Note**: Running the script requires root privileges to set weights.

## Notes

- **Memory Alignment**: Memory is aligned to 4096 bytes, so `benchmark_bytes` is rounded up accordingly.
- **GPU Limit**: Ensure `--ngpus` does not exceed available GPUs.
- **Concurrency**: With multiple GPUs, benchmarks run simultaneously and asynchronously, causing overlap.
- **NUMA Nodes**: Verify valid node indices with `numactl --hardware`.
- **Pinning**: `--pin_memory 1` improves performance but increases setup overhead.
- **NUMA Policies**: `numactl` enables flexible memory allocation (e.g., interleave, weighted interleave, node-specific).

## Extensions

The `extension/` directory contains small companion utilities.

1. [**numa-cpu-xfer-bench**](extension/numa-cpu-xfer-bench/README.md)  
   Wrapper around Intel Memory Latency Checker (MLC) for measuring bandwidth between CPU cores and NUMA node memory. Useful as a CPU only baseline when comparing against GPU transfer results.

2. [**numa-numa-xfer-bench**](extension/numa-numa-xfer-bench/README.md)  
   Standalone C++ and OpenMP benchmark that measures memory copy bandwidth between different NUMA nodes using `libnuma`. Useful to study pure NUMA to NUMA bandwidth.
