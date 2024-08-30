#pragma once

#include <cstdint>

namespace web {
    void* malloc(std::size_t size);
    void* realloc(void* ptr, std::size_t new_size);
    void* calloc(std::size_t count, std::size_t size);
    void free(void* ptr);
};