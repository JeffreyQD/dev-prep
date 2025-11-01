#ifndef __KIT_SPSC_HPP__
#define __KIT_SPSC_HPP__

#include <atomic>

namespace kit {

template <typename T, int N>
class SPSCRingBuffer {
 public:
  SPSCRingBuffer() : head_(0), tail_(0) {
    buffer_ = static_cast<T*>(::operator new(sizeof(T) * capacity_));
  }
  ~SPSCRingBuffer() {
    uint64_t head = head_.load(std::memory_order_relaxed);
    uint64_t tail = tail_.load(std::memory_order_relaxed);

    for (; head != tail; head = (head + 1) & mask_) {
      buffer_[head].~T();
    }
    ::operator delete(buffer_);
  }

  template <typename... Args>
  bool push(Args&&... args) {
    uint64_t tail = tail_.load(std::memory_order_relaxed);
    uint64_t next_tail = (tail + 1) & mask_;

    if (next_tail == head_.load(std::memory_order_acquire)) {
      return false;
    }

    new (&buffer_[tail]) T(std::forward<Args>(args)...);
    tail_.store(next_tail, std::memory_order_release);
    return true;
  }

  bool pop(T& entry) {
    uint64_t head = head_.load(std::memory_order_relaxed);

    if (head == tail_.load(std::memory_order_acquire)) {
      return false;
    }

    entry = std::move(buffer_[head]);
    buffer_[head].~T();
    head_.store((head + 1) & mask_, std::memory_order_release);
    return true;
  }

 private:
  T* buffer_;
  std::atomic<uint64_t> head_, tail_;
  static constexpr uint64_t capacity_ = 1ull << N;
  static constexpr uint64_t mask_ = capacity_ - 1;
};  // SPSCRingBuffer

}  // namespace kit

#endif