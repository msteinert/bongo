// Copyright The Go Authors.

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/os.h"

namespace bongo::os {

TEST_CASE("Stat", "[os]") {
  auto [file, err1] = open("/etc/group");
  REQUIRE(err1 == nil);
  auto [dir, err2] = file.stat();
  REQUIRE(err2 == nil);
  CHECK("group" == dir.name);
}

TEST_CASE("Stat error", "[os]") {
}

TEST_CASE("Fstat", "[os]") {
}

TEST_CASE("Fstat error", "[os]") {
}

}  // namespace bongo::os
