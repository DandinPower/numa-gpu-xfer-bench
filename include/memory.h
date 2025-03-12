#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

void* allocate_cpu_buffer(size_t size, const std::vector<int>& interleave_nodes, bool pin_memory);
void free_cpu_buffer(void* ptr, size_t size, bool pin_memory);

#endif