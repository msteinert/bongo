// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <chrono>

#include <catch2/catch.hpp>

#include "bongo/chan.h"
#include "bongo/time.h"

using namespace std::chrono_literals;
using namespace std::string_literals;

TEST_CASE("Expiration", "[time]") {
  bongo::time::timer t{1ms};
  decltype(t)::recv_type v;
  v << t.c();
  REQUIRE(v.has_value() == true);
  REQUIRE(v.value() >= 1ms);
}

TEST_CASE("Reset after stop", "[time]") {
  bongo::time::timer t{100ms};
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
  bongo::time::timer t{1ms};
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

TEST_CASE("Reset error", "[time]") {
  CHECK_THROWS_AS([]() {
    bongo::time::timer t{1ms};
    t.reset(2ms);
  }(), bongo::logic_error);
}

TEST_CASE("After function", "[time]") {
  bongo::chan<std::string> c;
  bongo::time::timer t{1ms, [&]() {
    c << "bingo bango bongo"s;
  }};
  decltype(c)::recv_type s;
  s << c;
  REQUIRE(s.has_value() == true);
  REQUIRE(*s == "bingo bango bongo");
}
