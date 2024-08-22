#include <circular_buffer.h>

template <class T>
bool circular_view<T>::put(T item) {
    if(!full()) {
        m_buf[m_head] = item;
        m_head = (m_head + 1) % m_max_size;
        return true;
    }
    return false;
}

template <class T>
size_t circular_view<T>::put(std::span<T> items) {
    size_t i = 0;
    for(i; i < items.size() && put(items[i]); i++);
    return i;
}

template <class T>
std::optional<T> circular_view<T>::get() {
    if(empty()) {
        return std::nullopt;
    }
    T val = m_buf[m_tail];
    m_tail = (m_tail + 1) % m_max_size;
    return val;
}

template <class T>
size_t circular_view<T>::get(std::span<T> &items) {
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

template <class T>
void circular_view<T>::reset() {
    m_head = m_tail;
}

template <class T>
bool circular_view<T>::empty() const {
    return m_head == m_tail;
}

template <class T>
bool circular_view<T>::full() const {
    return m_max_size == 0 || ((m_head + 1) % m_max_size) == m_tail;
}

template <class T>
size_t circular_view<T>::capacity() const {
    return m_max_size;
}

template <class T>
size_t circular_view<T>::size() const {
    size_t size = m_max_size;

	if(!full())
	{
		if(m_head >= m_tail)
		{
			size = m_head - m_tail;
		}
		else
		{
			size = m_max_size + m_head - m_tail;
		}
	}

	return size;
}

template <class T>
void circular_view<T>::advance(size_t amount) {
    m_tail = (m_tail + amount) % m_max_size;
}

template <class T>
circular_iterator<T> circular_view<T>::begin() const {
    return circular_iterator<T>(const_cast<unsigned char*>(m_buf), m_tail, m_max_size);
}

template <class T>
circular_iterator<T> circular_view<T>::end() const {
    return circular_iterator<T>(const_cast<unsigned char*>(m_buf), m_head + 1, m_max_size);
}

template class circular_view<uint8_t>;