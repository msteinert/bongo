// Copyright The Go Authors.

#include <chrono>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/time.h"

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace bongo::time {

TEST_CASE("Expiration", "[time]") {
  timer t{1ms};
  decltype(t)::recv_type v;
  v << t.c();
  REQUIRE(v.has_value() == true);
  REQUIRE(v.value() >= 1ms);
}

TEST_CASE("Reset after stop", "[time]") {
  timer t{1s};
  if (!t.stop()) {
    FAIL_CHECK("timer expired");
  }
  t.reset(1ms);
  decltype(t)::recv_type v;
  v << t.c();
  REQUIRE(v.has_value() == true);
  REQUIRE(v.value() >= 1ms);
}

TEST_CASE("Reset after timeout", "[time]") {
  timer t{1ms};
  std::this_thread::sleep_for(10ms);
  if (!t.stop()) {
    decltype(t)::recv_type v;
    v << t.c();
    REQUIRE(v.has_value() == true);
    REQUIRE(v.value() >= 1ms);
  } else {
    FAIL_CHECK("timer was stopped");
  }
  t.reset(2ms);
  decltype(t)::recv_type v;
  v << t.c();
  REQUIRE(v.has_value() == true);
  REQUIRE(v.value() >= 2ms);
}

TEST_CASE("After function", "[time]") {
  chan<std::string> c;
  timer t{1ms, [&]() {
    c << "bingo bango bongo"s;
  }};
  decltype(c)::recv_type s;
  s << c;
  REQUIRE(s.has_value() == true);
  REQUIRE(*s == "bingo bango bongo");
}

}  // namespace bongo::time
