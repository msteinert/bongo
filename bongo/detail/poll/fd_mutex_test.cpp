// Copyright The Go Authors.

#include <chrono>
#include <random>
#include <thread>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/detail/poll/fd_mutex.h"
#include "bongo/time.h"

using namespace std::chrono_literals;

namespace bongo::detail::poll {

TEST_CASE("Lock FD mutex", "[detail/poll]") {
  auto mu = fd_mutex{};
  REQUIRE(mu.incref() == true);
  REQUIRE(mu.decref() == false);
  REQUIRE(mu.rwlock(true) == true);
  REQUIRE(mu.rwunlock(true) == false);
  REQUIRE(mu.rwlock(false) == true);
  REQUIRE(mu.rwunlock(false) == false);
}

TEST_CASE("Close FD mutex", "[detail/poll]") {
  auto mu = fd_mutex{};
  REQUIRE(mu.incref_and_close() == true);
  REQUIRE(mu.incref() == false);
  REQUIRE(mu.rwlock(true) == false);
  REQUIRE(mu.rwlock(false) == false);
  REQUIRE(mu.incref_and_close() == false);
}

TEST_CASE("Close and unblock FD mutex", "[detail/poll]") {
  std::vector<std::thread> threads;
  auto c = chan<bool>{4};
  auto mu = fd_mutex{};
  mu.rwlock(true);
  for (auto i = 0; i < 4; ++i) {
    threads.emplace_back([&]() {
      if (mu.rwlock(true)) {
        c << false;  // broken
      } else {
        c << true;
      }
    });
  }

  std::this_thread::sleep_for(1ms);
  std::optional<bool> r;
  switch (select(
    recv_select_case(c, r),
    default_select_case()
  )) {
  case 0:
    FAIL("broken");
  default:
    break;
  }

  mu.incref_and_close();
  for (auto i = 0; i < 4; ++i) {
    time::timer timer{10s};
    decltype(timer)::recv_type trecv;
    switch (select(
      recv_select_case(c, r),
      recv_select_case(timer.c(), trecv)
    )) {
    case 0:
      if (r && *r == false) {
        FAIL("broken");
      }
      break;
    case 1:
      FAIL("broken");
    }
  }

  REQUIRE(mu.decref() == false);
  REQUIRE(mu.rwunlock(true) == true);

  for (auto& t : threads) {
    t.join();
  }
}

TEST_CASE("FD mutex exception", "[detail/poll]") {
  auto mu = fd_mutex{};
  CHECK_THROWS_AS([&]() {
    mu.decref();
  }(), std::logic_error);
  CHECK_THROWS_AS([&]() {
    mu.rwunlock(true);
  }(), std::logic_error);
  CHECK_THROWS_AS([&]() {
    mu.rwunlock(false);
  }(), std::logic_error);

  CHECK_THROWS_AS([&]() {
    mu.incref();
    mu.decref();
    mu.decref();
  }(), std::logic_error);
  CHECK_THROWS_AS([&]() {
    mu.rwlock(true);
    mu.rwunlock(true);
    mu.rwunlock(true);
  }(), std::logic_error);
  CHECK_THROWS_AS([&]() {
    mu.rwlock(false);
    mu.rwunlock(false);
    mu.rwunlock(false);
  }(), std::logic_error);

  mu.incref();
  mu.decref();
  mu.rwlock(true);
  mu.rwunlock(true);
  mu.rwlock(false);
  mu.rwunlock(false);
}

TEST_CASE("FD mutex overflow exception", "[detail/poll]") {
  CHECK_THROWS_AS([]() {
    auto mu = fd_mutex{};
    for (auto i = 0; i < (1<<21); ++i) {
      mu.incref();
    }
  }(), std::runtime_error);
}

TEST_CASE("FD mutex stress", "[detail/poll]") {
  size_t P = 8;
  auto N = static_cast<int>(1e3);

  auto mu = fd_mutex{};
  auto done = chan<bool>{P};
  std::vector<std::thread> threads;

  uint64_t read_state[2] = {0, 0};
  uint64_t write_state[2] = {0, 0};

  for (size_t p = 0; p < P; ++p) {
    threads.emplace_back([&]() {
      bool failed = false;
      auto _ = defer([&]() {
        done << !failed;
      });
      std::mt19937 gen{std::random_device{}()};
      auto r = std::uniform_int_distribution<>{0, 2};
      for (auto i = 0; i < N; ++i) {
        switch (r(gen)) {
        case 0:
          if (!mu.incref()) {
            failed = true;
            return;
          }
          if (mu.decref()) {
            failed = true;
            return;
          }
          break;
        case 1:
          if (!mu.rwlock(true)) {
            failed = true;
            return;
          }
          if (read_state[0] != read_state[1]) {
            failed = true;
            return;
          }
          ++read_state[0];
          ++read_state[1];
          if (mu.rwunlock(true)) {
            failed = true;
            return;
          }
          break;
        case 2:
          if (!mu.rwlock(false)) {
            failed = true;
            return;
          }
          if (write_state[0] != write_state[1]) {
            failed = true;
            return;
          }
          ++write_state[0];
          ++write_state[1];
          if (mu.rwunlock(false)) {
            failed = true;
            return;
          }
          break;
        }
      }
    });
  }

  bool r;
  for (size_t p = 0; p < P; ++p) {
    r << done;
    if (!r) {
      FAIL("broken");
    }
  }

  for (auto& t : threads) {
    t.join();
  }

  REQUIRE(mu.incref_and_close() == true);
  REQUIRE(mu.decref() == true);
}

}  // namespace bongo::detail::poll
