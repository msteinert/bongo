// Copyright The Go Authors.

#pragma once

#include <complex>
#include <iterator>
#include <limits>

#include <bongo/strconv/ftoa.h>

namespace bongo::strconv {

template <typename T, std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt format(std::complex<T> v, OutputIt out, char fmt = 'e', long prec = -1) {
  *out++ = '(';
  out = format<T, OutputIt>(v.real(), out, fmt, prec);
  if (v.imag() > 0 && v.imag() != std::numeric_limits<T>::infinity()) {
    *out++ = '+';
  }
  out = format<T, OutputIt>(v.imag(), out, fmt, prec);
  *out++ = 'i';
  *out++ = ')';
  return out;
}

}  // namespace bongo::strconv
