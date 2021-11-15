// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <bongo/math/bits.h>

namespace bongo::math {

constexpr uint32_t float_bits(float f) {
  return bits::bit_cast<uint32_t>(f);
}

constexpr uint64_t float_bits(double f) {
  return bits::bit_cast<uint64_t>(f);
}

}  // namespace bongo::math
