// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

namespace bongo::unicode {

using rune = char32_t;

static constexpr rune max_rune         = 0x0010ffff;
static constexpr rune replacement_char = 0xfffd;
static constexpr rune max_ascii        = 0x007f;
static constexpr rune max_latin1       = 0x00ff;

enum {
  upper_case = 0,
  lower_case,
  title_case,
  max_case
};

}  // namesapce bongo::unicode
