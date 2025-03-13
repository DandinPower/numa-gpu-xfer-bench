#include "memory.h"

void* allocate_cpu_buffer(size_t size, bool pin_memory) {
    void* ptr = nullptr;
    int ret = posix_memalign(&ptr, DMA_ALIGNMENT, size);
    if (ret != 0) {
        std::cerr << "Failed to allocate aligned memory. posix_memalign returned: " << std::to_string(ret) << std::endl;
        exit(EXIT_FAILURE);
    }
    std::memset(ptr, 0, size);
    if (pin_memory) {
        CUDA_CHECK(cudaHostRegister(ptr, size, cudaHostRegisterPortable));
    }
    return ptr;
}

void free_cpu_buffer(void* ptr, bool pin_memory) {
    if (pin_memory) {
        CUDA_CHECK(cudaHostUnregister(ptr));
    }
    free(ptr);
}