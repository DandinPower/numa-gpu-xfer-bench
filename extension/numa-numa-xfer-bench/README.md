sudo apt install libnuma-dev

# Compile & Run
The compilation command remains the same:

```bash
g++ -o main main.cpp -lnuma -fopenmp -O3
```

# New Example Run:

To test a 4GB copy from an interleaved buffer on nodes 0 and 1 to an interleaved buffer on nodes 2 and 3, using 16 threads:

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
./main --src_nodes "0-1" --dst_nodes "2-3" ...
```