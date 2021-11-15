// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <string_view>

namespace bongo::strconv {
namespace detail {

static constexpr std::string_view lower_hex = "0123456789abcdef";
static constexpr std::string_view upper_hex = "0123456789ABCDEF";

}  // namespace detail
}  // namespace bongo::strconv
