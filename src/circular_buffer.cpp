#include "circular_buffer.h"

template <class T, size_t count>
bool circular_buffer<T, count>::put(T item) {
    if(!full()) {
        buf_[head_] = item;
        head_ = (head_ + 1) % max_size_;
        return true;
    }
    return false;
}

template <class T, size_t count>
size_t circular_buffer<T, count>::put(std::span<T> items) {
    size_t i = 0;
    for(i; i < items.size() && put(items[i]); i++);
    return i;
}

template <class T, size_t count>
std::optional<T> circular_buffer<T, count>::get() {
    if(empty()) {
        return std::nullopt;
    }
    T val = buf_[tail_];
    tail_ = (tail_ + 1) % max_size_;
    return val;
}

template <class T, size_t count>
size_t circular_buffer<T, count>::get(std::span<T> &items) {
    size_t i = 0;
    for(i; i < items.size(); i++) {
        std::optional<T> val = get();
        if(!val) {
            break;
        }
        items[i] = *val;
    }
    return i;
}

template <class T, size_t count>
void circular_buffer<T, count>::reset() {
    head_ = tail_;
}

template <class T, size_t count>
bool circular_buffer<T, count>::empty() const {
    return head_ == tail_;
}

template <class T, size_t count>
bool circular_buffer<T, count>::full() const {
    return ((head_ + 1) % max_size_) == tail_;
}

template <class T, size_t count>
size_t circular_buffer<T, count>::capacity() const {
    return max_size_;
}

template <class T, size_t count>
size_t circular_buffer<T, count>::size() const {
    size_t size = max_size_;

	if(!full())
	{
		if(head_ >= tail_)
		{
			size = head_ - tail_;
		}
		else
		{
			size = max_size_ + head_ - tail_;
		}
	}

	return size;
}

template <class T, size_t count>
void circular_buffer<T, count>::advance(size_t amount) {
    tail_ = (tail_ + amount) % max_size_;
}

template <class T, size_t count>
circular_buffer<T, count>::iterator circular_buffer<T, count>::begin() const {
    return iterator(const_cast<unsigned char*>(buf_), tail_, max_size_);
}

template <class T, size_t count>
circular_buffer<T, count>::iterator circular_buffer<T, count>::end() const {
    return iterator(const_cast<unsigned char*>(buf_), head_ + 1, max_size_);
}

template <class T, size_t count>
circular_buffer<T, count>::iterator::iterator(pointer ptr, size_t index, size_t max_size): ptr_(ptr), index_(index), max_size_(max_size) {}

template <class T, size_t count>
circular_buffer<T, count>::iterator::reference circular_buffer<T, count>::iterator::operator*() const {
    return ptr_[index_ % max_size_];
}

template <class T, size_t count>
circular_buffer<T, count>::iterator::pointer circular_buffer<T, count>::iterator::operator->() {
    return &ptr_[index_ % max_size_];
}

template <class T, size_t count>
circular_buffer<T, count>::iterator& circular_buffer<T, count>::iterator::operator++() {
    index_++;
    return *this;
}

template <class T, size_t count>
circular_buffer<T, count>::iterator circular_buffer<T, count>::iterator::operator++(int) {
    iterator tmp = *this;
    index_++;
    return tmp;
}

template <class T, size_t count>
circular_buffer<T, count>::iterator circular_buffer<T, count>::iterator::operator+(int rhs) {
    iterator tmp(ptr_, index_ + rhs, max_size_);
    return tmp;
}

template <class T, size_t count>
bool circular_buffer<T, count>::iterator::operator==(const iterator& rhs) {
    return ((std::ptrdiff_t)(ptr_ + index_) % max_size_) == ((std::ptrdiff_t)(rhs.ptr_ + rhs.index_) % max_size_);
}

template <class T, size_t count>
bool circular_buffer<T, count>::iterator::operator!=(const iterator& rhs) {
    return !(*this == rhs);
}

template class circular_buffer<uint8_t, 2048>;