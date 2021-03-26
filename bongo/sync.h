// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <condition_variable>
#include <mutex>

namespace bongo::sync {

/**
 * Wait groups can be used to synchronize groups of threads.
 *
 * - https://golang.org/pkg/sync/#WaitGroup
 */
class wait_group {
  std::mutex mutex_;
  std::condition_variable cond_;
  int state_;

 public:
  wait_group()
      : wait_group{0} {}
  wait_group(int n)
      : state_{n} {}

  void add(int n);
  void done();
  void wait();
};

}  // namesapce bongo::sync
