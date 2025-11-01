#include <gtest/gtest.h>

#include <atomic>
#include <concepts>
#include <future>
#include <iostream>
#include <kit/ringbuffer/mpmc.hpp>
#include <kit/ringbuffer/spmc.hpp>
#include <kit/ringbuffer/spsc.hpp>
#include <thread>
#include <type_traits>
#include <vector>

template <typename Buffer, typename T, typename... Args>
concept RingBuffer = requires(Buffer buffer, T entry, Args&&... args) {
  { buffer.push(std::forward<Args>(args)...) } -> std::same_as<bool>;
  { buffer.pop(entry) } -> std::same_as<bool>;
};

template <typename Buffer>
  requires RingBuffer<Buffer, int>
void producer_func(Buffer& buffer, int n) {
  for (int i = 0; i < n; ++i) {
    while (!buffer.push(i)) {
      std::this_thread::yield();
    }
  }
}

template <typename Buffer>
  requires RingBuffer<Buffer, int>
uint64_t consumer_func(Buffer& buffer, int n) {
  uint64_t res = 0;
  int entry;

  for (int i = 0; i < n; ++i) {
    while (!buffer.pop(entry)) {
      std::this_thread::yield();
    }
    res += entry;
  }
  return res;
}

TEST(RingBuffer, SPSC) {
  kit::SPSCRingBuffer<int, 10> buffer;
  int n = 1000000;
  std::jthread producer(producer_func<decltype(buffer)>, std::ref(buffer), n);

  std::promise<uint64_t> p;
  std::future<uint64_t> res = p.get_future();

  std::jthread consumer([&p, &buffer, &n]() {
    uint64_t ret = consumer_func<decltype(buffer)>(buffer, n);
    p.set_value(ret);
  });

  uint64_t ans = static_cast<uint64_t>(n) * (n - 1) / 2;
  ASSERT_EQ(res.get(), ans);
}

TEST(RingBuffer, MPMC) {
  kit::MPMCRingBuffer<int, 10> buffer;
  int n = 1000000, num_threads = 16;
  std::vector<std::jthread> producers;
  for (int i = 0; i < num_threads; ++i) {
    producers.emplace_back(producer_func<decltype(buffer)>, std::ref(buffer),
                           n);
  }

  std::vector<std::future<uint64_t>> consumer_res;
  for (int i = 0; i < num_threads; ++i) {
    std::packaged_task<uint64_t(decltype(buffer)&, int)> task(
        consumer_func<decltype(buffer)>);
    consumer_res.emplace_back(task.get_future());
    std::jthread t(std::move(task), std::ref(buffer), n);
  }

  uint64_t res = 0;
  for (int i = 0; i < num_threads; ++i) {
    res += consumer_res[i].get();
  }

  uint64_t ans = static_cast<uint64_t>(n) * (n - 1) / 2 * num_threads;
  ASSERT_EQ(res, ans);
}

TEST(RingBuffer, SPMC) {
  kit::SPMCRingBuffer<int, 10> buffer;
  int n = 1000000, num_consumers = 32;

  std::jthread producer(producer_func<decltype(buffer)>, std::ref(buffer),
                        n * num_consumers);

  std::vector<std::future<uint64_t>> consumer_res;
  for (int i = 0; i < num_consumers; ++i) {
    std::packaged_task<uint64_t(decltype(buffer)&, int)> task(
        consumer_func<decltype(buffer)>);
    consumer_res.emplace_back(task.get_future());
    std::jthread t(std::move(task), std::ref(buffer), n);
  }

  uint64_t res = 0;
  for (int i = 0; i < num_consumers; ++i) {
    res += consumer_res[i].get();
  }

  n *= num_consumers;
  uint64_t ans = static_cast<uint64_t>(n) * (n - 1) / 2;
  ASSERT_EQ(res, ans);
}