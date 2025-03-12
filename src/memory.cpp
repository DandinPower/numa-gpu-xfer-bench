#include "memory.h"

void* allocate_cpu_buffer(size_t size, const std::vector<int>& interleave_nodes, bool pin_memory) {
    struct bitmask* nodemask = numa_allocate_nodemask();
    if (!nodemask) {
        std::cerr << "Failed to allocate nodemask." << std::endl;
        exit(EXIT_FAILURE);
    }
    numa_bitmask_clearall(nodemask);
    int max_node = numa_max_node();
    for (auto node : interleave_nodes) {
        if (node < 0 || node > max_node) continue;
        numa_bitmask_setbit(nodemask, node);
    }
    void* ptr = numa_alloc_interleaved_subset(size, nodemask);
    numa_free_nodemask(nodemask);
    if (ptr == nullptr) {
        std::cerr << "Failed to allocate memory." << std::endl;
        exit(EXIT_FAILURE);
    }
    std::memset(ptr, 0, size);
    if (pin_memory) {
        CUDA_CHECK(cudaHostRegister(ptr, size, cudaHostRegisterMapped));
    }
    return ptr;
}

void free_cpu_buffer(void* ptr, size_t size, bool pin_memory) {
    if (pin_memory) {
        CUDA_CHECK(cudaHostUnregister(ptr));
    }
    numa_free(ptr, size);
}