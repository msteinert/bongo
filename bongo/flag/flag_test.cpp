// Copyright The Go Authors.

#include "bongo/flag.h"

#include <catch2/catch.hpp>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

#include "bongo/bongo.h"

namespace bongo::flag {

using namespace std::string_view_literals;

TEST_CASE("Flag", "[flag]") {
  auto arguments = std::vector<std::string>{"-x"};
  auto f = flag_set{"test"};
  f.parse(arguments);
}

}  // namespace bongo::flag
