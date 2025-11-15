# numa-cpu-xfer-bench

This extension uses Intel Memory Latency Checker (MLC) to measure bandwidth between CPU cores and NUMA node memory. It is useful as a CPU only baseline when comparing with GPU transfer results from `numa-gpu-xfer-bench`.

Official MLC page:  
https://www.intel.com/content/www/us/en/developer/articles/tool/intelr-memory-latency-checker.html

The binary `mlc` in this directory is Intel MLC v3.12.

## Prerequisites

- A system with multiple NUMA nodes.
- `numactl` installed.
- Root privileges for some tests.

To prepare 1000 x 2 MiB huge pages on each odes 0 and 1 (assume you have 2 nodes):

```bash
echo 1000 | sudo tee /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
echo 1000 | sudo tee /sys/devices/system/node/node1/hugepages/hugepages-2048kB/nr_hugepages
```

Adjust the counts and node IDs according to your system.

## Basic usage

Run the bandwidth matrix across all nodes:

```bash
./mlc --bandwidth_matrix
```

This reports a matrix of bandwidth values where rows and columns correspond to source and target NUMA nodes. It is a good high level view of your system topology.

## Per node bandwidth example

To measure peak injection bandwidth when allocating memory only on a specific NUMA node, combine `numactl` with MLC. For example, for node 1:

```bash
sudo numactl --membind=1 ./mlc --peak_injection_bandwidth
```

Key points:

* `--membind` forces allocations to the selected NUMA node.
* `--peak_injection_bandwidth` stresses the memory subsystem and reports peak bandwidth.

Use different `--membind` values to compare nodes and correlate with GPU and NUMA transfer results from the main benchmark.