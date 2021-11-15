// Copyright Exegy, Inc.
// Copyright The Go Authors.

#include <string_view>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/strconv.h"

using namespace std::string_view_literals;

TEST_CASE("Decimal", "[strconv]") {
  SECTION("Shift") {
    auto test_cases = std::vector<std::tuple<uint64_t, long, std::string_view>>{
      {0, -100, "0"},
      {0, 100, "0"},
      {1, 100, "1267650600228229401496703205376"},
      {1, -100, "0.0000000000000000000000000000007888609052210118054117285652827862296732064351090230047702789306640625"},
      {12345678, 8, "3160493568"},
      {12345678, -8, "48225.3046875"},
      {195312, 9, "99999744"},
      {1953125, 9, "1000000000"},
    };
    for (auto [in, shift, exp] : test_cases) {
      auto d = bongo::strconv::detail::decimal{};
      d.assign(in);
      d.shift(shift);
      auto out = std::string{};
      d.str(std::back_inserter(out));
      CHECK(out == exp);
    }
  }
  SECTION("Round") {
    auto test_cases = std::vector<std::tuple<
      uint64_t,
      long,
      std::string_view,
      std::string_view,
      std::string_view,
      uint64_t
    >>{
      {0, 4, "0", "0", "0", 0},
      {12344999, 4, "12340000", "12340000", "12350000", 12340000},
      {12345000, 4, "12340000", "12340000", "12350000", 12340000},
      {12345001, 4, "12340000", "12350000", "12350000", 12350000},
      {23454999, 4, "23450000", "23450000", "23460000", 23450000},
      {23455000, 4, "23450000", "23460000", "23460000", 23460000},
      {23455001, 4, "23450000", "23460000", "23460000", 23460000},

      {99994999, 4, "99990000", "99990000", "100000000", 99990000},
      {99995000, 4, "99990000", "100000000", "100000000", 100000000},
      {99999999, 4, "99990000", "100000000", "100000000", 100000000},

      {12994999, 4, "12990000", "12990000", "13000000", 12990000},
      {12995000, 4, "12990000", "13000000", "13000000", 13000000},
      {12999999, 4, "12990000", "13000000", "13000000", 13000000},
    };
    for (auto [in, nd, down, round, up, i] : test_cases) {
      CAPTURE(in, nd, i);
      {
        auto d = bongo::strconv::detail::decimal{in};
        d.round_down(nd);
        std::string out;
        d.str(std::back_inserter(out));
        CHECK(out == down);
      }
      {
        auto d = bongo::strconv::detail::decimal{in};
        d.round(nd);
        std::string out;
        d.str(std::back_inserter(out));
        CHECK(out == round);
      }
      {
        auto d = bongo::strconv::detail::decimal{in};
        d.round_up(nd);
        std::string out;
        d.str(std::back_inserter(out));
        CHECK(out == up);
      }
    }
  }
  SECTION("Rounded integer") {
    auto test_cases = std::vector<std::tuple<uint64_t, long, uint64_t>>{
      {0, 100, 0},
      {512, -8, 2},
      {513, -8, 2},
      {640, -8, 2},
      {641, -8, 3},
      {384, -8, 2},
      {385, -8, 2},
      {383, -8, 1},
      {1, 100, std::numeric_limits<uint64_t>::max()},
      {1000, 0, 1000},
    };
    for (auto [in, shift, exp] : test_cases) {
        CAPTURE(in, shift);
        auto d = bongo::strconv::detail::decimal{in};
        d.shift(shift);
        CHECK(d.rounded_integer() == exp);
    }
  }
}
