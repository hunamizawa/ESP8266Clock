/**
 * @file CircularQueue.h
 */

#ifndef CircularQueue_H_
#define CircularQueue_H_

#include <deque>
#include <stddef.h>

/**
 * @brief キャパシティを超えると勝手に pop される queue
 * 
 * @tparam T 
 * @tparam Capacity 要素数の上限
 */
template <class T, size_t Capacity>
class CircularQueue {
private:
  using TIterator      = typename std::deque<T>::iterator;
  using TConstIterator = typename std::deque<T>::const_iterator;

protected:
  std::deque<T> c = std::deque<T>(Capacity);

public:
  void fill(const T &value) {
    c.assign(Capacity, value);
  }

  bool empty() const {
    return c.empty();
  }

  size_t size() const {
    return c.size();
  }

  T &front() {
    return c.front();
  }

  const T &front() const {
    return c.front();
  }

  T &back() {
    return c.back();
  }

  const T &back() const {
    return c.back();
  }

  void push_back(const T &x) {
    while (c.size() >= Capacity)
      c.pop_front();
    c.push_back(x);
  }

  void push_back(T &&y) {
    while (c.size() >= Capacity)
      c.pop_front();
    c.push_back(y);
  }

  void pop_front() {
    c.pop_front();
  }

  void clear() noexcept {
    c.clear();
  }

  TIterator begin() {
    return c.begin();
  }

  TConstIterator begin() const {
    return c.begin();
  }

  TIterator end() {
    return c.end();
  }

  TConstIterator end() const {
    return c.end();
  }
};

#endif // CircularQueue_H_