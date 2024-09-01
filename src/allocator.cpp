#include <allocator.h>

#include <bitset>
#include <math.h>
#include <cstring>

#ifndef WEB_ALLOC_CHUNK_SIZE
#define WEB_ALLOC_CHUNK_SIZE 16
#endif

#ifndef WEB_ALLOC_POOL_SIZE
#define WEB_ALLOC_POOL_SIZE 65536
#endif

#define WEB_ALLOC_CHUNK_COUNT (WEB_ALLOC_POOL_SIZE/WEB_ALLOC_CHUNK_SIZE)

static std::bitset<WEB_ALLOC_CHUNK_COUNT> allocations{};
static uint8_t data[WEB_ALLOC_POOL_SIZE];
const uint8_t* data_start = &data[0];
const uint8_t* data_end = data_start + WEB_ALLOC_POOL_SIZE;

namespace web::impl {
    void* malloc(std::size_t size);
    void* realloc(void* ptr, std::size_t new_size);
    void* calloc(std::size_t count, std::size_t size);
    void free(void* ptr);
}

void* web::impl::malloc(std::size_t size) {
    if(size == 0) {
        return nullptr;
    }
    std::size_t chunks_needed = ceil(((float)(size + sizeof(std::size_t))) / WEB_ALLOC_CHUNK_SIZE);
    std::size_t start = 0, end = 0, offset;
    if(allocations.size() - allocations.count() < chunks_needed) {
        return nullptr;
    }
    for(std::size_t i = 0; i < allocations.size(); i++) {
        if(allocations[i]) {
            start = i + 1;
            continue;
        }
        if((i - start) == chunks_needed) {
            end = i;
            break;
        }
    }
    if(end == 0) {
        return nullptr;
    }
    for(std::size_t i = start; i < end; i++) {
        allocations.set(i);
    }
    offset = WEB_ALLOC_CHUNK_SIZE * start;
    *(std::size_t*)(&data[offset]) = chunks_needed;
    return &data[offset + sizeof(std::size_t)];
}

void* web::impl::calloc(std::size_t count, std::size_t size) {
    void* to_return = web::impl::malloc(count * size);
    if(to_return == nullptr) {
        return nullptr;
    }
    std::memset(to_return, 0, count * size);
    return to_return;
}

void* web::impl::realloc(void* ptr, std::size_t new_size) {
    if(ptr == nullptr) {
        return web::impl::malloc(new_size);
    }
    if((((uint8_t*)ptr) - sizeof(std::size_t)) < data_start || ptr >= data_end) {
        // We don't manage memory outside of our pool
        return nullptr;
    }
    const std::size_t new_chunk_count = ceil((new_size + sizeof(std::size_t)) / WEB_ALLOC_CHUNK_SIZE);
    const std::size_t old_chunk_count = *(std::size_t*)(((uint8_t*)ptr) - sizeof(std::size_t));
    const std::size_t chunk_start = (std::size_t)((uint8_t*)(((uint8_t*)ptr) - sizeof(std::size_t)) - data_start) / WEB_ALLOC_CHUNK_SIZE;
    if(new_chunk_count == old_chunk_count) {
        return ptr;
    }
    if(new_chunk_count < old_chunk_count) {
        for(std::size_t i = chunk_start + new_chunk_count; i < chunk_start + old_chunk_count; i++) {
            allocations.reset(i);
        }
        return ptr;
    }
    bool must_move = false;
    for(std::size_t i = chunk_start + old_chunk_count; i < chunk_start + new_chunk_count; i++) {
        if(allocations[i]) {
            must_move = true;
            break;
        }
    }
    if(must_move) {
        void* new_ptr = web::impl::malloc(new_size);
        if(new_ptr == nullptr) {
            return nullptr;
        }
        const std::size_t old_size = old_chunk_count * WEB_ALLOC_CHUNK_SIZE - sizeof(std::size_t);
        std::memcpy(new_ptr, ptr, old_size);
        web::impl::free(ptr);
        return new_ptr;
    }
    for(std::size_t i = chunk_start + old_chunk_count; i < chunk_start + new_chunk_count; i++) {
        allocations.set(i);
    }
    *(std::size_t*)(((uint8_t*)ptr) - sizeof(std::size_t)) = new_chunk_count;
    return ptr;
}

void web::impl::free(void* ptr) {
    if(ptr == nullptr) {
        return;
    }
    if((((uint8_t*)ptr) - sizeof(std::size_t)) < data_start || ptr >= data_end) {
        // We don't manage memory outside of our pool
        return;
    }
    const std::size_t chunk_count = *(std::size_t*)(((uint8_t*)ptr) - sizeof(std::size_t));
    const std::size_t chunk_start = (std::size_t)((uint8_t*)(((uint8_t*)ptr) - sizeof(std::size_t)) - data_start) / WEB_ALLOC_CHUNK_SIZE;
    for(std::size_t i = chunk_start; i < chunk_count + chunk_start; i++) {
        allocations.reset(i);
    }
}

void* web::malloc(std::size_t size) {
    void* to_return = nullptr;
    mutex_enter_blocking(&web_malloc_mutex);
    to_return = web::impl::malloc(size);
    mutex_exit(&web_malloc_mutex);
    return to_return;
}

void* web::calloc(std::size_t count, std::size_t size) {
    void* to_return = nullptr;
    mutex_enter_blocking(&web_malloc_mutex);
    to_return = web::impl::calloc(count, size);
    mutex_exit(&web_malloc_mutex);
    return to_return;
}

void* web::realloc(void* ptr, std::size_t new_size) {
    void* to_return = nullptr;
    mutex_enter_blocking(&web_malloc_mutex);
    to_return = web::impl::realloc(ptr, new_size);
    mutex_exit(&web_malloc_mutex);
    return to_return;
}

void web::free(void* ptr) {
    if(ptr == nullptr) {
        return;
    }
    mutex_enter_blocking(&web_malloc_mutex);
    web::impl::free(ptr);
    mutex_exit(&web_malloc_mutex);
}
