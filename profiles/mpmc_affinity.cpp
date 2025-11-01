#include <benchmark/benchmark.h>
#include <pthread.h>

#include <atomic>
#include <future>
#include <iostream>
#include <kit/ringbuffer/mpmc.hpp>
#include <kit/utils.hpp>
#include <thread>
#include <vector>

template <typename Queue>
void producer(Queue& q, int threadIdx, int n, int cpu = -1) {
  if (cpu != -1) {
    kit::set_affinity(pthread_self(), cpu);
  }
  for (int i = 0; i < n; ++i) {
    while (!q.push(i));
  }
}

template <typename Queue>
uint64_t consumer(Queue& q, int threadIdx, int n, int cpu = -1) {
  if (cpu != -1) {
    kit::set_affinity(pthread_self(), cpu);
  }
  uint64_t res = 0;
  int entry;
  for (int i = 0; i < n; ++i) {
    while (!q.pop(entry));
    res += entry;
  }
  return res;
}

static void BM_MPMC(benchmark::State& state) {
  int num_threads = state.range(0);
  for (auto _ : state) {
    kit::MPMCRingBuffer<int, 11> q;
    int n = 10000;
    std::vector<std::thread> producers;
    for (int i = 0; i < num_threads; ++i) {
      producers.emplace_back(producer<decltype(q)>, std::ref(q), i, n, -1);
    }
    std::vector<std::future<uint64_t>> rets;
    for (int i = 0; i < num_threads; ++i) {
      rets.emplace_back(
          std::async(consumer<decltype(q)>, std::ref(q), i, n, -1));
    }

    uint64_t res = 0;
    for (int i = 0; i < num_threads; ++i) {
      res += rets[i].get();
    }
    for (auto& t : producers) {
      t.join();
    }
  }
}

static void BM_MPMC_affinity(benchmark::State& state) {
  int num_threads = state.range(0);
  int num_cpus = std::thread::hardware_concurrency();
  for (auto _ : state) {
    kit::MPMCRingBuffer<int, 11> q;
    int n = 10000;
    std::vector<std::thread> producers;
    for (int i = 0; i < num_threads; ++i) {
      producers.emplace_back(producer<decltype(q)>, std::ref(q), i, n, 2);
    }
    std::vector<std::future<uint64_t>> rets;
    for (int i = 0; i < num_threads; ++i) {
      rets.emplace_back(
          std::async(consumer<decltype(q)>, std::ref(q), i, n, 3));
    }

    uint64_t res = 0;
    for (int i = 0; i < num_threads; ++i) {
      res += rets[i].get();
    }
    for (auto& t : producers) {
      t.join();
    }
  }
}

BENCHMARK(BM_MPMC)->Arg(4)->Arg(8)->Arg(16)->Arg(32)->Arg(64)->Arg(128);
BENCHMARK(BM_MPMC_affinity)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Arg(128);

BENCHMARK_MAIN();
