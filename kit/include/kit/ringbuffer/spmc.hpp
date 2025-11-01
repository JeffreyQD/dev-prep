#ifndef __KIT_RINGBUFFER_SPMC_HPP__
#define __KIT_RINGBUFFER_SPMC_HPP__

#include <atomic>

namespace kit {

template <typename T, int N>
class SPMCRingBuffer {
 public:
  SPMCRingBuffer() : head_(0), tail_(0) {
    buffer_ = static_cast<Slot*>(::operator new(sizeof(Slot) * capacity_));
    for (uint64_t idx = 0; idx != capacity_; ++idx) {
      buffer_[idx].seq.store(idx, std::memory_order_relaxed);
    }
  }
  ~SPMCRingBuffer() {
    uint64_t head = head_.load(std::memory_order_relaxed);
    uint64_t tail = tail_.load(std::memory_order_relaxed);

    for (; head != tail; ++head) {
      buffer_[head & mask_].data.~T();
    }
    ::operator delete(buffer_);
  }
  SPMCRingBuffer(const SPMCRingBuffer&) = delete;
  SPMCRingBuffer& operator=(const SPMCRingBuffer&) = delete;
  SPMCRingBuffer(SPMCRingBuffer&&) = delete;
  SPMCRingBuffer& operator=(SPMCRingBuffer&&) = delete;

  template <typename... Args>
  bool push(Args&&... args) {
    uint64_t tail = tail_.load(std::memory_order_relaxed);
    Slot* slot = &buffer_[tail & mask_];

    int64_t diff = static_cast<int64_t>(slot->seq) - static_cast<int64_t>(tail);

    if (diff < 0) {
      // The buffer is full.
      return false;
    }

    new (&slot->data) T(std::forward<Args>(args)...);
    slot->seq.store(tail + 1, std::memory_order_release);
    tail_.store(tail + 1, std::memory_order_release);
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
      } else {
        // This slot has been poppped by other threads.
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
  alignas(64) std::atomic<uint64_t> head_;
  alignas(64) std::atomic<uint64_t> tail_;
  static constexpr uint64_t capacity_ = 1ull << N;
  static constexpr uint64_t mask_ = capacity_ - 1;
};  // SPMCRingBuffer

}  // namespace kit

#endif