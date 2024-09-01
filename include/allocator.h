#pragma once

#include <cstdint>
#include <type_traits>

#include "pico/mutex.h"
auto_init_mutex(web_malloc_mutex);

namespace web {
    void* malloc(std::size_t size);
    void* realloc(void* ptr, std::size_t new_size);
    void* calloc(std::size_t count, std::size_t size);
    void free(void* ptr);

    template <class T>
    struct allocator {
        using value_type = T;

        allocator() noexcept {}
        template <class U> allocator(const allocator<U>&) noexcept {}

        T* allocate(std::size_t count) {
            return (T*)web::malloc(count * sizeof(T));
        }

        void deallocate(T* ptr, std::size_t count) {
            web::free(ptr);
        }
    };
};

template <class T, class U>
constexpr bool operator==(const web::allocator<T>&, const web::allocator<U>&) noexcept {
    return std::is_same<T,U>::value;
}

template <class T, class U>
constexpr bool operator!=(const web::allocator<T>&, const web::allocator<U>&) noexcept {
    return !std::is_same<T,U>::value;
}