// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <string>
#include <string_view>

#include <bongo/unicode/utf8.h>
#include <bongo/strconv/detail/quote.h>

namespace bongo::strconv {

constexpr bool can_rawquote(std::string_view s) {
  return detail::can_rawquote(s.begin(), s.end());
}

template <std::output_iterator<uint8_t> OutputIt>
OutputIt quote(std::string_view in, OutputIt out) {
  return detail::quote_with(in.begin(), in.end(), out, '"', false, false);
}

template <std::output_iterator<uint8_t> OutputIt>
OutputIt quote(rune in, OutputIt out) {
  return detail::escape_rune(out, in, false, false);
}

inline std::string quote(std::string_view in) {
  return detail::quote_with(in, '"', false, false);
}

template <std::output_iterator<uint8_t> OutputIt>
OutputIt quote_to_ascii(std::string_view in, OutputIt out) {
  return detail::quote_with(in.begin(), in.end(), out, '"', true, false);
}

inline std::string quote_to_ascii(std::string_view in) {
  return detail::quote_with(in, '"', true, false);
}

template <std::output_iterator<uint8_t> OutputIt>
OutputIt quote_to_ascii(rune in, OutputIt out) {
  return detail::escape_rune(out, in, true, false);
}

}  // namespace bongo::strconv
