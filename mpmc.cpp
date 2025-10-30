#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <barrier>
#include <cmath>
#include <coroutine>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <semaphore>
#include <stop_token>
#include <string_view>
#include <syncstream>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
auto file_sink =
    std::make_shared<spdlog::sinks::basic_file_sink_mt>("log.txt", true);

spdlog::logger logger("logger", {console_sink, file_sink});

template <typename T>
class MPMC {
 public:
  MPMC(int n)
      : mask_((1ull << n) - 1),
        capacity_(1 << n),
        head_(0),
        tail_(0),
        cnt_(0),
        total_cnt_(0) {
    buffer_ = static_cast<Slot*>(::operator new(sizeof(Slot) * capacity_));
    for (uint64_t idx = 0; idx != capacity_; ++idx) {
      buffer_[idx].seq.store(idx, std::memory_order_relaxed);
    }
  }

  template <typename... Args>
  bool push(Args&&... args) {
    Slot* slot;
    uint64_t tail = tail_.load(std::memory_order_acquire);
    while (true) {
      slot = &buffer_[tail & mask_];
      uint64_t seq = slot->seq.load(std::memory_order_acquire);
      int64_t diff = static_cast<int64_t>(seq) - static_cast<int64_t>(tail);
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
    uint64_t cnt = cnt_.fetch_add(1, std::memory_order_release);
    uint64_t total_cnt = total_cnt_.fetch_add(1, std::memory_order_release);

    logger.info("[push] | {} | seq = {} | {} | {}", tail, tail + 1, cnt + 1,
                total_cnt + 1);
    return true;
  }

  bool pop(T& entry) {
    Slot* slot;
    uint64_t head = head_.load(std::memory_order_acquire);
    while (true) {
      slot = &buffer_[head & mask_];
      uint64_t seq = slot->seq;
      int64_t diff = static_cast<int64_t>(seq) - static_cast<int64_t>(head + 1);
      if (diff == 0) {
        if (head_.compare_exchange_strong(head, head + 1)) {
          break;
        }
      } else if (diff < 0) {
        // The buffer is empty.
        return false;
      } else {
        // This slot has been popped by other threads.
        head = head_.load(std::memory_order_acquire);
      }
    }

    entry = std::move(slot->data);
    slot->data.~T();
    slot->seq.store(head + capacity_, std::memory_order_release);
    uint64_t cnt = cnt_.fetch_sub(1, std::memory_order_release);
    logger.info("[pop] {} | seq = {} | {} | {}", head, head + capacity_,
                cnt - 1, total_cnt_.load(std::memory_order_acquire));
    return true;
  }

 private:
  struct Slot {
    T data;
    std::atomic<uint64_t> seq;
  };
  Slot* buffer_;
  uint64_t capacity_;
  uint64_t mask_;
  std::atomic<uint64_t> cnt_, total_cnt_;
  std::atomic<uint64_t> head_, tail_;
};

void producer(MPMC<int>& q, int threadIdx, int n) {
  for (int i = 0; i < n; ++i) {
    while (!q.push(i));
  }
  logger.info("producer {} done", threadIdx);
}

uint64_t consumer(MPMC<int>& q, int threadIdx, int n) {
  uint64_t res = 0;
  int entry;
  for (int i = 0; i < n; ++i) {
    while (!q.pop(entry));
    res += entry;
  }
  logger.info("consumer {} done", threadIdx);
  return res;
}

int main() {
  MPMC<int> q(11);
  int num_threads = 128, n = 10000;
  std::vector<std::thread> producers;
  for (int i = 0; i < num_threads; ++i) {
    producers.emplace_back(producer, std::ref(q), i, n);
  }
  std::vector<std::future<uint64_t>> rets;
  for (int i = 0; i < num_threads; ++i) {
    rets.emplace_back(std::async(consumer, std::ref(q), i, n));
  }

  uint64_t res = 0;
  for (int i = 0; i < num_threads; ++i) {
    res += rets[i].get();
  }
  for (auto& t : producers) {
    t.join();
  }
}
