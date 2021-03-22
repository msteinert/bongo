// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <any>
#include <chrono>
#include <optional>

#include <catch2/catch.hpp>

#include "bongo/context.h"
#include "bongo/time.h"

using namespace std::chrono_literals;
using namespace std::string_literals;

TEST_CASE("Background context", "[context]") {
  auto c = bongo::context::background();
  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(c->done(), v),
    bongo::default_select_case(),
  })) {
  case 0:
    FAIL_CHECK("unexpected value");
    break;
  default:
    break;
  }
}

TEST_CASE("TODO context", "[context]") {
  auto c = bongo::context::todo();
  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(c->done(), v),
    bongo::default_select_case(),
  })) {
  case 0:
    FAIL_CHECK("unexpected value");
    break;
  default:
    break;
  }
}

TEST_CASE("With cancel context", "[context]") {
  auto [c1, cancel1] = bongo::context::with_cancel(bongo::context::background());
  auto [c2, cancel2] = bongo::context::with_cancel(c1);

  auto check1 = [](bongo::context::context& c) {
    REQUIRE(c.done() != nullptr);
    REQUIRE(c.err() == nullptr);
    std::optional<std::monostate> v;
    switch (bongo::select({
      bongo::recv_select_case(c.done(), v),
      bongo::default_select_case(),
    })) {
    case 0:
      FAIL_CHECK("unexpected value");
      break;
    default:
      break;
    }
  };

  auto check2 = [](bongo::context::context& c) {
    std::optional<std::monostate> v;
    switch (bongo::select({
      bongo::recv_select_case(c.done(), v),
      bongo::default_select_case(),
    })) {
    case 0:
      break;
    default:
      FAIL_CHECK("done blocked");
      break;
    }
    REQUIRE(c.err() == bongo::context::canceled);
  };

  check1(*c1);
  check1(*c2);

  cancel1();
  check2(*c1);
  check2(*c2);
}

TEST_CASE("Timeout context", "[context]") {
  auto [c, cancel] = bongo::context::with_timeout(bongo::context::background(), 1ms);
  auto t = bongo::time::timer{5s};

  std::optional<std::chrono::system_clock::duration> d;
  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(t.c(), d),
    bongo::recv_select_case(c->done(), v),
  })) {
  case 0:
    FAIL_CHECK("context not timed out after 5s");
    return;
  case 1:
    break;
  }
  REQUIRE(c->err() == bongo::context::deadline_exceeded);
}

TEST_CASE("Wrapped timeout context", "[context]") {
  auto [c1, cancel1] = bongo::context::with_timeout(bongo::context::background(), 1ms);
  auto [c2, cancel2] = bongo::context::with_timeout(c1, 1min);
  auto t = bongo::time::timer{5s};

  std::optional<std::chrono::system_clock::duration> d;
  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(t.c(), d),
    bongo::recv_select_case(c2->done(), v),
  })) {
  case 0:
    FAIL_CHECK("context not timed out after 5s");
    return;
  case 1:
    break;
  }
  REQUIRE(c2->err() == bongo::context::deadline_exceeded);
}

TEST_CASE("Canceled timeout context", "[context]") {
  auto [c, cancel] = bongo::context::with_timeout(bongo::context::background(), 1min);
  cancel();
  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(c->done(), v),
    bongo::default_select_case(),
  })) {
  case 0:
    break;
  default:
    FAIL_CHECK("context blocked");
  }
  REQUIRE(c->err() == bongo::context::canceled);
}

TEST_CASE("Deadline context", "[context]") {
  auto [c, cancel] = bongo::context::with_deadline(
      bongo::context::background(), std::chrono::system_clock::now() + 1ms);
  auto t = bongo::time::timer{5s};

  std::optional<std::chrono::system_clock::duration> d;
  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(t.c(), d),
    bongo::recv_select_case(c->done(), v),
  })) {
  case 0:
    FAIL_CHECK("context not timed out after 5s");
    return;
  case 1:
    break;
  }
  REQUIRE(c->err() == bongo::context::deadline_exceeded);
}

TEST_CASE("Wrapped deadline context", "[context]") {
  auto [c1, cancel1] = bongo::context::with_deadline(
      bongo::context::background(), std::chrono::system_clock::now() + 1ms);
  auto [c2, cancel2] = bongo::context::with_deadline(
      c1, std::chrono::system_clock::now() + 1min);  // NOLINT
  auto t = bongo::time::timer{5s};

  std::optional<std::chrono::system_clock::duration> d;
  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(t.c(), d),
    bongo::recv_select_case(c2->done(), v),
  })) {
  case 0:
    FAIL_CHECK("context not timed out after 5s");
  case 1:
    break;
  }
  REQUIRE(c2->err() == bongo::context::deadline_exceeded);  // NOLINT
}

TEST_CASE("Canceled deadline context", "[context]") {
  auto [c, cancel] = bongo::context::with_deadline(
      bongo::context::background(), std::chrono::system_clock::now() + 1min);
  cancel();
  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(c->done(), v),
    bongo::default_select_case(),
  })) {
  case 0:
    break;
  default:
    FAIL_CHECK("context blocked");
  }
  REQUIRE(c->err() == bongo::context::canceled);
}

TEST_CASE("Value context", "[context]") {
  auto key1 = std::string{"key1"};
  auto key2 = std::string{"key2"};
  auto key3 = std::string{"key3"};

  auto c0 = bongo::context::background();
  REQUIRE(c0->value(key1).has_value() == false);
  REQUIRE(c0->value(key2).has_value() == false);
  REQUIRE(c0->value(key3).has_value() == false);

  auto c1 = bongo::context::with_value(c0, key1, "c1k1"s);
  REQUIRE(c1->value(key1).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c1->value(key1)) == "c1k1"s);
  REQUIRE(c1->value(key2).has_value() == false);
  REQUIRE(c1->value(key3).has_value() == false);

  auto c2 = bongo::context::with_value(c1, key2, "c2k2"s);
  REQUIRE(c2->value(key1).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c2->value(key1)) == "c1k1"s);
  REQUIRE(c2->value(key2).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c2->value(key2)) == "c2k2"s);
  REQUIRE(c2->value(key3).has_value() == false);

  auto c3 = bongo::context::with_value(c2, key3, "c3k3"s);
  REQUIRE(c3->value(key1).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c3->value(key1)) == "c1k1"s);
  REQUIRE(c3->value(key2).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c3->value(key2)) == "c2k2"s);
  REQUIRE(c3->value(key3).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c3->value(key3)) == "c3k3"s);
}

TEST_CASE("Parent cancels child", "[context]") {
  auto [parent, cancel_parent] = bongo::context::with_cancel(bongo::context::background());
  auto [child, cancel_child] = bongo::context::with_cancel(parent);
  auto value = bongo::context::with_value(parent, "key"s, "value"s);
  auto [timer, cancel_timer] = bongo::context::with_timeout(value, 5min);

  std::optional<std::monostate> v;
  switch (bongo::select({
    bongo::recv_select_case(parent->done(), v),
    bongo::recv_select_case(child->done(), v),
    bongo::recv_select_case(value->done(), v),
    bongo::recv_select_case(timer->done(), v),  // NOLINT
    bongo::default_select_case(),
  })) {
  case 0:
    FAIL_CHECK("cancel parent should block");
  case 1:
    FAIL_CHECK("cancel child should block");
  case 2:
    FAIL_CHECK("value should block");
  case 3:
    FAIL_CHECK("timer should block");
  default:
    break;
  }

  cancel_parent();
  auto check = [](bongo::context::context& c) {
    std::optional<std::monostate> v;
    switch (bongo::select({
      bongo::recv_select_case(c.done(), v),
      bongo::default_select_case(),
    })) {
    case 0:
      break;
    default:
      FAIL_CHECK("canceled context should not block");
    }
    REQUIRE(c.err() == bongo::context::canceled);
  };
  check(*parent);
  check(*child);
  check(*value);  // NOLINT
  check(*timer);  // NOLINT

  auto precanceled = bongo::context::with_value(parent, "key"s, "value"s);
  switch (bongo::select({
    bongo::recv_select_case(precanceled->done(), v),
    bongo::default_select_case(),
  })) {
  case 0:
    break;
  default:
    FAIL_CHECK("pre-canceled context should not block");
  }
  REQUIRE(precanceled->err() == bongo::context::canceled);
}

TEST_CASE("Child does not cancel parent", "[context]") {
  auto check = [](std::shared_ptr<bongo::context::context> parent) {
    auto [child, cancel] = bongo::context::with_cancel(parent);
    std::optional<std::monostate> v;
    switch (bongo::select({
      bongo::recv_select_case(parent->done(), v),
      bongo::recv_select_case(child->done(), v),
      bongo::default_select_case(),
    })) {
    case 0:
      FAIL_CHECK("parent should block");
    case 1:
      FAIL_CHECK("child should block");
    default:
      break;
    }
    cancel();
    switch (bongo::select({
      bongo::recv_select_case(child->done(), v),
      bongo::default_select_case(),
    })) {
    case 0:
      break;
    default:
      FAIL_CHECK("child should not block");
    }
    REQUIRE(child->err() == bongo::context::canceled);
    switch (bongo::select({
      bongo::recv_select_case(parent->done(), v),
      bongo::default_select_case(),
    })) {
    case 0:
      FAIL_CHECK("parent should block");
    default:
      break;
    }
    REQUIRE(parent->err() == nullptr);
  };

  check(bongo::context::background());
  auto [ctx, cancel] = bongo::context::with_cancel(bongo::context::background());
  check(ctx);
}
