#pragma once

#include <memory>
#include <optional>
#include <span>

#include <iterator>


template <class T>
struct circular_iterator {
    //using iterator_concept = std::input_iterator;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;

    circular_iterator() = delete;
    circular_iterator(pointer ptr, size_t index, size_t max_size);

    reference operator*() const;
    pointer operator->();

    circular_iterator& operator++();
    circular_iterator operator++(int);

    circular_iterator operator+(int rhs);

    bool operator==(const circular_iterator& rhs);
    bool operator!=(const circular_iterator& rhs);

private:
    pointer ptr_;
    size_t index_, max_size_;
};

template <class T>
class circular_base {
public:
    virtual bool put(T item) = 0;
    virtual size_t put(std::span<T> items) = 0;
    virtual std::optional<T> get() = 0;
    virtual size_t get(std::span<T> &items) = 0;
    virtual void reset() = 0;
    virtual bool empty() const = 0;
    virtual bool full() const = 0;
    virtual size_t capacity() const = 0;
    virtual size_t size() const = 0;

    virtual void advance(size_t amount) = 0;

    virtual circular_iterator<T> begin() const = 0;
    virtual circular_iterator<T> end() const = 0;
};

template <class T, size_t count>
class circular_buffer : circular_base<T> {
public:
    bool put(T item) override;
    size_t put(std::span<T> items) override;
    std::optional<T> get() override;
    size_t get(std::span<T> &items) override;
    void reset() override;
    bool empty() const override;
    bool full() const override;
    size_t capacity() const override;
    size_t size() const override;

    void advance(size_t amount) override;

    circular_iterator<T> begin() const override;
    circular_iterator<T> end() const override;

private:
    T buf_[count];
    size_t head_ = 0;
    size_t tail_ = 0;
    const size_t max_size_ = count;
};

template <class T>
class circular_view : circular_base<T> {
public:
    circular_view() = default;
    circular_view(T* start, size_t length) : m_buf(start), m_max_size(length) {}

    bool put(T item) override;
    size_t put(std::span<T> items) override;
    std::optional<T> get() override;
    size_t get(std::span<T> &items) override;
    void reset() override;
    bool empty() const override;
    bool full() const override;
    size_t capacity() const override;
    size_t size() const override;

    void advance(size_t amount) override;

    circular_iterator<T> begin() const override;
    circular_iterator<T> end() const override;

private:
    T* m_buf = nullptr;
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_max_size = 0;
};