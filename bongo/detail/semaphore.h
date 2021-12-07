// Copyright The Go Authors.

#pragma once

#include <limits.h>
#include <semaphore.h>

namespace bongo::detail {

template <ptrdiff_t Max = SEM_VALUE_MAX>
class semaphore {
  sem_t sem_;

 public:
  semaphore() {
    errno = 0;
    if (sem_init(&sem_, 0, Max) == -1) {
      throw std::system_error{std::error_code{errno, std::system_category()}};
    }
  }

  ~semaphore() {
    sem_destroy(&sem_);
  }

  semaphore(semaphore const& other) = delete;
  semaphore(semaphore&& other) = delete;
  semaphore& operator=(semaphore&& other) = delete;
  semaphore& operator=(semaphore const& other) = delete;

  void release(ptrdiff_t update = 1) {
    for (; update; --update) {
      sem_post(&sem_);
    }
  }

  void acquire() {
    sem_wait(&sem_);
  }
};

}  // namespace bongo::detail
