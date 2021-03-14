// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <atomic>
#include <thread>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/chan.h"
#include "bongo/sync.h"

void test_wait_group(bongo::sync::wait_group& wg1, bongo::sync::wait_group& wg2) {
  std::vector<std::thread> threads;
  size_t n = 16;
  wg1.add(n);
  wg2.add(n);
  auto exited = bongo::chan<bool>{n};
  for (size_t i = 0; i < n; ++i) {
    threads.emplace_back([&]() {
        wg1.done();
        wg2.wait();
        exited << true;
    });
  }
  wg1.wait();
  std::optional<bool> e;
  for (size_t i = 0; i < n; ++i) {
    switch (bongo::select({
      bongo::recv_select_case(exited, e),
      bongo::default_select_case(),
    })) {
    case 0:
      FAIL_CHECK("released group too soon");
      break;
    case 1:
      break;
    }
    wg2.done();
  }
  for (size_t i = 0; i < n; ++i) {
    e << exited;
  }
  for (auto& t : threads) {
    t.join();
  }
  SUCCEED("groups synchronized");
}

TEST_CASE("Test wait group", "[sync]") {
  auto wg1 = bongo::sync::wait_group{};
  auto wg2 = bongo::sync::wait_group{};
  for (size_t i = 0; i < 8; ++i) {
    test_wait_group(wg1, wg2);
  }
}

TEST_CASE("Wait group misuse", "[sync]") {
  CHECK_THROWS_AS([]() {
    auto wg = bongo::sync::wait_group{};
    wg.add(1);
    wg.done();
    wg.done();
  }(), bongo::logic_error);
}
