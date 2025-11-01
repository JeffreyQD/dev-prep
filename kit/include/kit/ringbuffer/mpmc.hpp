#ifndef __KIT_RINGBUFFER_MPMPC_HPP__
#define __KIT_RINGBUFFER_MPMPC_HPP__

#include <atomic>
#include <new>

namespace kit {

template <typename T, int N>
class MPMCRingBuffer {
 public:
  MPMCRingBuffer() : head_(0), tail_(0) {
    buffer_ = static_cast<Slot*>(::operator new(sizeof(Slot) * capacity_));
    for (uint64_t idx = 0; idx != capacity_; ++idx) {
      buffer_[idx].seq.store(idx, std::memory_order_relaxed);
    }
  }
  ~MPMCRingBuffer() {
    uint64_t head = head_.load(std::memory_order_relaxed);
    uint64_t tail = tail_.load(std::memory_order_relaxed);

    for (; head != tail; ++head) {
      buffer_[head].data.~T();
    }

    ::operator delete(buffer_);
  }

  template <typename... Args>
  bool push(Args&&... args) {
    Slot* slot;
    uint64_t tail = tail_.load(std::memory_order_acquire);

    while (true) {
      slot = &buffer_[tail & mask_];
      int64_t diff =
          static_cast<int64_t>(slot->seq) - static_cast<int64_t>(tail);

      if (diff == 0) {
        if (tail_.compare_exchange_strong(tail, tail + 1)) {
          break;
        }
      } else if (diff < 0) {
        // The buffer is full.
        return false;
      } else {
        // This slot has been pushed by other threads.
        tail = tail_.load(std::memory_order_acquire);
      }
    }

    new (&slot->data) T(std::forward<Args>(args)...);
    slot->seq.store(tail + 1, std::memory_order_release);
    return true;
  }

  bool pop(T& entry) {
    Slot* slot;
    uint64_t head = head_.load(std::memory_order_acquire);

    while (true) {
      slot = &buffer_[head & mask_];
      int64_t diff =
          static_cast<int64_t>(slot->seq) - static_cast<int64_t>(head + 1);
      if (diff == 0) {
        if (head_.compare_exchange_strong(head, head + 1)) {
          break;
        }
      } else if (diff < 0) {
        // The buffer is empty.
        return false;
      } else {
        head = head_.load(std::memory_order_acquire);
      }
    }

    entry = std::move(slot->data);
    slot->data.~T();
    slot->seq.store(head + capacity_, std::memory_order_release);
    return true;
  }

 private:
  struct Slot {
    std::atomic<uint64_t> seq;
    T data;
  };

  Slot* buffer_;
  static constexpr uint64_t capacity_ = (1ull << N);
  static constexpr uint64_t mask_ = (1ull << N) - 1;
  alignas(64) std::atomic<uint64_t> head_;
  alignas(64) std::atomic<uint64_t> tail_;
};  // MPMCRingBuffer

}  // namespace kit

#endif
