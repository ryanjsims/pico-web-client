#include <circular_buffer.h>

template <class T>
circular_iterator<T>::circular_iterator(pointer ptr, size_t index, size_t max_size): ptr_(ptr), index_(index), max_size_(max_size) {}

template <class T>
circular_iterator<T>::reference circular_iterator<T>::operator*() const {
    return ptr_[index_ % max_size_];
}

template <class T>
circular_iterator<T>::pointer circular_iterator<T>::operator->() {
    return &ptr_[index_ % max_size_];
}

template <class T>
circular_iterator<T>& circular_iterator<T>::operator++() {
    index_++;
    return *this;
}

template <class T>
circular_iterator<T> circular_iterator<T>::operator++(int) {
    circular_iterator tmp = *this;
    index_++;
    return tmp;
}

template <class T>
circular_iterator<T> circular_iterator<T>::operator+(int rhs) {
    circular_iterator tmp(ptr_, index_ + rhs, max_size_);
    return tmp;
}

template <class T>
bool circular_iterator<T>::operator==(const circular_iterator& rhs) {
    return ((std::ptrdiff_t)(ptr_ + index_) % max_size_) == ((std::ptrdiff_t)(rhs.ptr_ + rhs.index_) % max_size_);
}

template <class T>
bool circular_iterator<T>::operator!=(const circular_iterator& rhs) {
    return !(*this == rhs);
}

template class circular_iterator<uint8_t>;