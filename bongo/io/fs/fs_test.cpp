// Copyright The Go Authors.

#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/fmt.h"
#include "bongo/io/fs.h"

namespace bongo::io::fs {

TEST_CASE("File mode types", "[io/fs]") {
  auto test_cases = std::vector<std::tuple<file_mode, std::string>>{
    {mode_dir,         "d---------"},
    {mode_append,      "a---------"},
    {mode_exclusive,   "l---------"},
    {mode_temporary,   "T---------"},
    {mode_symlink,     "L---------"},
    {mode_device,      "D---------"},
    {mode_named_pipe,  "p---------"},
    {mode_socket,      "S---------"},
    {mode_setuid,      "u---------"},
    {mode_setgid,      "g---------"},
    {mode_char_device, "c---------"},
    {mode_sticky,      "t---------"},
    {mode_irregular,   "?---------"},
  };
  for (auto& [mode, exp] : test_cases) {
    auto s = fmt::to_string(mode);
    CHECK(s == exp);
  }
}

TEST_CASE("File mode perm", "[io/fs]") {
  auto test_cases = std::vector<std::tuple<file_mode, std::string>>{
    {0777, "-rwxrwxrwx"},
    {0755, "-rwxr-xr-x"},
  };
  for (auto& [mode, exp] : test_cases) {
    auto s = fmt::to_string(mode);
    CHECK(s == exp);
  }
}

}  // namespace bongo::io::fs
