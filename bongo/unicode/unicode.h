// Copyright The Go Authors.

#pragma once

#include <bongo/bongo.h>
#include <bongo/unicode/utf16.h>
#include <bongo/unicode/utf8.h>

namespace bongo::unicode {

static constexpr rune max_rune         = 0x10ffff;
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
