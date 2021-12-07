// Copyright The Go Authors.

#include <any>
#include <chrono>
#include <optional>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/context.h"
#include "bongo/time.h"

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace bongo::context {

TEST_CASE("Background context", "[context]") {
  auto c = background();
  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(c->done(), v),
    default_select_case(),
  })) {
  case 0:
    FAIL_CHECK("unexpected value");
    break;
  default:
    break;
  }
}

TEST_CASE("TODO context", "[context]") {
  auto c = todo();
  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(c->done(), v),
    default_select_case(),
  })) {
  case 0:
    FAIL_CHECK("unexpected value");
    break;
  default:
    break;
  }
}

TEST_CASE("With cancel context", "[context]") {
  auto [c1, cancel1] = with_cancel(background());
  auto r2 = with_cancel(c1);
  auto c2 = std::get<0>(r2);

  auto check1 = [](context& c) {
    REQUIRE(c.done() != nullptr);
    REQUIRE(c.err() == nil);
    std::optional<std::monostate> v;
    switch (select({
      recv_select_case(c.done(), v),
      default_select_case(),
    })) {
    case 0:
      FAIL_CHECK("unexpected value");
      break;
    default:
      break;
    }
  };

  auto check2 = [](context& c) {
    std::optional<std::monostate> v;
    switch (select({
      recv_select_case(c.done(), v),
      default_select_case(),
    })) {
    case 0:
      break;
    default:
      FAIL_CHECK("done blocked");
      break;
    }
    REQUIRE(c.err() == error::canceled);
  };

  check1(*c1);
  check1(*c2);

  cancel1();
  check2(*c1);
  check2(*c2);
}

TEST_CASE("Timeout context", "[context]") {
  auto r = with_timeout(background(), 1ms);
  auto c = std::get<0>(r);
  auto t = time::timer{5s};

  std::optional<std::chrono::system_clock::duration> d;
  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(t.c(), d),
    recv_select_case(c->done(), v),
  })) {
  case 0:
    FAIL_CHECK("context not timed out after 5s");
    return;
  case 1:
    break;
  }
  REQUIRE(c->err() == error::deadline_exceeded);
}

TEST_CASE("Wrapped timeout context", "[context]") {
  auto r1 = with_timeout(background(), 1ms);
  auto c1 = std::get<0>(r1);
  auto r2 = with_timeout(c1, 1min);
  auto c2 = std::get<0>(r2);
  auto t = time::timer{5s};

  std::optional<std::chrono::system_clock::duration> d;
  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(t.c(), d),
    recv_select_case(c2->done(), v),
  })) {
  case 0:
    FAIL_CHECK("context not timed out after 5s");
    return;
  case 1:
    break;
  }
  REQUIRE(c2->err() == error::deadline_exceeded);
}

TEST_CASE("Canceled timeout context", "[context]") {
  auto [c, cancel] = with_timeout(background(), 1min);
  cancel();
  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(c->done(), v),
    default_select_case(),
  })) {
  case 0:
    break;
  default:
    FAIL_CHECK("context blocked");
  }
  REQUIRE(c->err() == error::canceled);
}

TEST_CASE("Deadline context", "[context]") {
  auto r = with_deadline(background(), std::chrono::system_clock::now() + 1ms);
  auto c = std::get<0>(r);
  auto t = time::timer{5s};

  std::optional<std::chrono::system_clock::duration> d;
  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(t.c(), d),
    recv_select_case(c->done(), v),
  })) {
  case 0:
    FAIL_CHECK("context not timed out after 5s");
    return;
  case 1:
    break;
  }
  REQUIRE(c->err() == error::deadline_exceeded);
}

TEST_CASE("Wrapped deadline context", "[context]") {
  auto r1 = with_deadline(background(), std::chrono::system_clock::now() + 1ms);
  auto c1 = std::get<0>(r1);
  auto r2 = with_deadline(
      c1, std::chrono::system_clock::now() + 1min);  // NOLINT
  auto c2 = std::get<0>(r2);
  auto t = time::timer{5s};

  std::optional<std::chrono::system_clock::duration> d;
  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(t.c(), d),
    recv_select_case(c2->done(), v),
  })) {
  case 0:
    FAIL_CHECK("context not timed out after 5s");
  case 1:
    break;
  }
  REQUIRE(c2->err() == error::deadline_exceeded);  // NOLINT
}

TEST_CASE("Canceled deadline context", "[context]") {
  auto [c, cancel] = with_deadline(
      background(), std::chrono::system_clock::now() + 1min);
  cancel();
  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(c->done(), v),
    default_select_case(),
  })) {
  case 0:
    break;
  default:
    FAIL_CHECK("context blocked");
  }
  REQUIRE(c->err() == error::canceled);
}

TEST_CASE("Value context", "[context]") {
  auto key1 = std::string{"key1"};
  auto key2 = std::string{"key2"};
  auto key3 = std::string{"key3"};

  auto c0 = background();
  REQUIRE(c0->value(key1).has_value() == false);
  REQUIRE(c0->value(key2).has_value() == false);
  REQUIRE(c0->value(key3).has_value() == false);

  auto c1 = with_value(c0, key1, "c1k1"s);
  REQUIRE(c1->value(key1).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c1->value(key1)) == "c1k1"s);
  REQUIRE(c1->value(key2).has_value() == false);
  REQUIRE(c1->value(key3).has_value() == false);

  auto c2 = with_value(c1, key2, "c2k2"s);
  REQUIRE(c2->value(key1).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c2->value(key1)) == "c1k1"s);
  REQUIRE(c2->value(key2).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c2->value(key2)) == "c2k2"s);
  REQUIRE(c2->value(key3).has_value() == false);

  auto c3 = with_value(c2, key3, "c3k3"s);
  REQUIRE(c3->value(key1).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c3->value(key1)) == "c1k1"s);
  REQUIRE(c3->value(key2).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c3->value(key2)) == "c2k2"s);
  REQUIRE(c3->value(key3).has_value() == true);
  REQUIRE(std::any_cast<std::string>(c3->value(key3)) == "c3k3"s);
}

TEST_CASE("Parent cancels child", "[context]") {
  auto [parent, cancel_parent] = with_cancel(background());
  auto child = std::get<0>(with_cancel(parent));
  auto value = with_value(parent, "key"s, "value"s);
  auto timer = std::get<0>(with_timeout(value, 5min));

  std::optional<std::monostate> v;
  switch (select({
    recv_select_case(parent->done(), v),
    recv_select_case(child->done(), v),
    recv_select_case(value->done(), v),
    recv_select_case(timer->done(), v),  // NOLINT
    default_select_case(),
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
  auto check = [](context& c) {
    std::optional<std::monostate> v;
    switch (select({
      recv_select_case(c.done(), v),
      default_select_case(),
    })) {
    case 0:
      break;
    default:
      FAIL_CHECK("canceled context should not block");
    }
    REQUIRE(c.err() == error::canceled);
  };
  check(*parent);
  check(*child);
  check(*value);  // NOLINT
  check(*timer);  // NOLINT

  auto precanceled = with_value(parent, "key"s, "value"s);
  switch (select({
    recv_select_case(precanceled->done(), v),
    default_select_case(),
  })) {
  case 0:
    break;
  default:
    FAIL_CHECK("pre-canceled context should not block");
  }
  REQUIRE(precanceled->err() == error::canceled);
}

TEST_CASE("Child does not cancel parent", "[context]") {
  auto check = [](std::shared_ptr<context> parent) {
    auto [child, cancel] = with_cancel(parent);
    std::optional<std::monostate> v;
    switch (select({
      recv_select_case(parent->done(), v),
      recv_select_case(child->done(), v),
      default_select_case(),
    })) {
    case 0:
      FAIL_CHECK("parent should block");
    case 1:
      FAIL_CHECK("child should block");
    default:
      break;
    }
    cancel();
    switch (select({
      recv_select_case(child->done(), v),
      default_select_case(),
    })) {
    case 0:
      break;
    default:
      FAIL_CHECK("child should not block");
    }
    REQUIRE(child->err() == error::canceled);
    switch (select({
      recv_select_case(parent->done(), v),
      default_select_case(),
    })) {
    case 0:
      FAIL_CHECK("parent should block");
    default:
      break;
    }
    REQUIRE(parent->err() == nil);
  };

  check(background());
  auto ctx = std::get<0>(with_cancel(background()));
  check(ctx);
}

}  // namespace bongo::context
