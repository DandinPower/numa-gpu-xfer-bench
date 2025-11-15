# numa-numa-xfer-bench

This extension is a pure CPU benchmark for measuring memory copy bandwidth between different NUMA nodes using `libnuma` and OpenMP. It allocates source and destination buffers with interleaved NUMA policies, then measures multi threaded `memcpy` between them.

This is useful when you want to:

- Understand NUMA to NUMA bandwidth without GPUs.
- Compare CPU only NUMA bandwidth against CPU GPU transfer bandwidth from `numa-gpu-xfer-bench`.
- Study the effects of NUMA placement and thread count.

## Prerequisite

Install `libnuma` development headers:

```bash
sudo apt install libnuma-dev
```

On non Debian systems, install the equivalent package that provides `<numa.h>` and `-lnuma`.

## Compile

From this directory:

```bash
g++ -o main main.cpp -lnuma -fopenmp -O3
```

This produces the `main` binary.

## Usage

The benchmark accepts the following arguments:

* `--src_nodes`
  Node list or range for the source buffer, for example `"0,1"` or `"0-3"`. Parsed by `numa_parse_nodestring`.

* `--dst_nodes`
  Node list or range for the destination buffer, same format as `--src_nodes`.

* `--benchmark_bytes`
  Total number of bytes to copy in each iteration (for example `4294967296` for 4 GiB).

* `--iterations_per_bench`
  Number of timed iterations.

* `--num_threads`
  Number of OpenMP threads to use.

If any required argument is missing, the program prints a usage message and exits.

## Example run

To test a 4 GiB copy from an interleaved source buffer on nodes 0 and 1 to an interleaved destination buffer on nodes 2 and 3, using 16 threads:

```bash
./main \
    --src_nodes "0,1" \
    --dst_nodes "2,3" \
    --benchmark_bytes 4294967296 \
    --iterations_per_bench 10 \
    --num_threads 16
```

You can also use ranges:

```bash
./main \
    --src_nodes "0-1" \
    --dst_nodes "2-3" \
    --benchmark_bytes 4294967296 \
    --iterations_per_bench 10 \
    --num_threads 16
```

## Output

For each run, the benchmark prints configuration and results similar to:

```text
--- Configuration ---
Source Nodes: 0,1
Destination Nodes: 2,3
Buffer Size: 4096 MiB
Iterations: 10
OpenMP Threads: 16
---------------------
--- Results ---
Average Latency: 12.345 ms
Average Bandwidth: 45.678 GiB/s
---------------
```

Interpretation:

* `Average Latency` is the average copy time for one full `benchmark_bytes` transfer across all iterations.
* `Average Bandwidth` is computed as `bytes / time` and reported in GiB per second, where GiB is 2^30 bytes.

Use these numbers to compare:

* Different NUMA node pairs.
* Different thread counts.
* CPU to CPU bandwidth versus CPU GPU bandwidth from the main benchmark.