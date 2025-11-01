#ifndef __KIT_UTILS_HPP__
#define __KIT_UTILS_HPP__

#include <pthread.h>

namespace kit {

bool set_affinity(pthread_t t, int cpu) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  int rc = pthread_setaffinity_np(t, sizeof(cpu_set_t), &cpuset);
  return rc == 0;
}

}  // namespace kit

#endif