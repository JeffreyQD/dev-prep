#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <new>
#include <random>
#include <syncstream>
#include <thread>
#include <utility>
#include <vector>

#include "kit/smart_pointer.hpp"

int main() {
  auto p1 = kit::make_shared<int>(42);
  // std::cout << sizeof(std::weak_ptr<int>) << "\n";
}