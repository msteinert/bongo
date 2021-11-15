// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <complex>
#include <limits>

#include <bongo/strconv/ftoa.h>

namespace bongo::strconv {

template <typename T, typename OutputIt>
constexpr void format(std::complex<T> v, OutputIt out, char fmt = 'e', long prec = -1) {
  *out++ = '(';
  format<T, OutputIt>(v.real(), out, fmt, prec);
  if (v.imag() > 0 && v.imag() != std::numeric_limits<T>::infinity()) {
    *out++ = '+';
  }
  format<T, OutputIt>(v.imag(), out, fmt, prec);
  *out++ = 'i';
  *out++ = ')';
}

}  // namespace bongo::strconv
