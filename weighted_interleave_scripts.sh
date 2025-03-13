#!/bin/bash
LOCAL_NODE=0
CXL_NODE=3  # Node 3 in my machine is a CXL memory node, while the other is a CPU node.
numa_nodes="$LOCAL_NODE,$CXL_NODE"

ITERATION_PER_BENCH=100
benchmark_bytes_items=("4096" "8192" "16384" "32768" "65536" "131072" "262144" "524288" "1048576" "2097152" "4194304" "8388608" "16777216" "33554432" "67108864" "134217728" "268435456" "536870912" "1073741824" "2147483648")
ngpus_items=("1" "2")
numa_weight_items=("1,1" "1,2" "1,3" "1,4") # This specifies weights for memory interleaving: local_node:weight1, cxl_node:weight2

[[ ! -x "./build/benchmark" ]] && {
    echo "Error: ./build/benchmark not found or not executable"
    exit 1
}

echo "ngpus,numa_nodes,numa_weight,access_size(bytes),read_latency(ms),read_bw(gib/s),write_latency(ms),write_bw(gib/s)"

for ngpus in "${ngpus_items[@]}"
do
    for numa_weight in "${numa_weight_items[@]}"
    do
        local_node_weight=$(echo "$numa_weight" | awk -F',' '{print $1}')
        cxl_node_weight=$(echo "$numa_weight" | awk -F',' '{print $2}')

        echo "$local_node_weight" > /sys/kernel/mm/mempolicy/weighted_interleave/node$LOCAL_NODE || {
            echo "Error: Failed to set weight for node $LOCAL_NODE"
            exit 1
        }
        echo "$cxl_node_weight" > /sys/kernel/mm/mempolicy/weighted_interleave/node$CXL_NODE || {
            echo "Error: Failed to set weight for node $CXL_NODE"
            exit 1
        }

        for benchmark_bytes in "${benchmark_bytes_items[@]}"
        do
            output=$(numactl --weighted-interleave="$numa_nodes" ./build/benchmark \
                --ngpus "$ngpus" \
                --benchmark_bytes "$benchmark_bytes" \
                --iterations_per_bench "$ITERATION_PER_BENCH" \
                --operation_type R,W \
                --pin_memory 1)

            h2d_latency=$(echo "$output" | grep "Average host-to-device latency" | awk '{print $4}')
            h2d_bandwidth=$(echo "$output" | grep "Average host-to-device bandwidth" | awk '{print $4}')
            d2h_latency=$(echo "$output" | grep "Average device-to-host latency" | awk '{print $4}')
            d2h_bandwidth=$(echo "$output" | grep "Average device-to-host bandwidth" | awk '{print $4}')

            echo "$ngpus,\"$numa_nodes\",\"$numa_weight\",$benchmark_bytes,$h2d_latency,$h2d_bandwidth,$d2h_latency,$d2h_bandwidth"
        done
    done
done