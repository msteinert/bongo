// Copyright The Go Authors.

#pragma once

#include <concepts>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <bongo/bongo.h>

namespace bongo::encoding {

// BinaryMarshaler is the interface implemented by an object that can marshal
// itself into a binary form.
//
// MarshalBinary encodes the receiver into a binary form and returns the
// result.
template <typename T>
concept BinaryMarshaler = requires (T m) {
  { m.marshal_binary() } -> std::same_as<std::pair<std::vector<uint8_t>, std::error_code>>;
};

template <BinaryMarshaler T>
std::pair<std::vector<uint8_t>, std::error_code> marshal_binary(T& m) {
  return m.marshal_binary();
}

template <typename T>
concept BinaryMarshalerFunc = requires (T m) {
  { marshal_binary(m) } -> std::same_as<std::pair<std::vector<uint8_t>, std::error_code>>;
};

// BinaryUnmarshaler is the interface implemented by an object that can
// unmarshal a binary representation of itself.
//
// UnmarshalBinary must be able to decode the form generated by MarshalBinary.
// UnmarshalBinary must copy the data if it wishes to retain the data after
// returning.
template <typename T>
concept BinaryUnmarshaler = requires (T m, std::span<uint8_t const> b) {
  { m.unmarshal_binary(b) } -> std::same_as<std::error_code>;
};

template <BinaryUnmarshaler T>
std::error_code unmarshal_binary(T& m, std::span<uint8_t const> b) {
  return m.unmarshal_binary(b);
}

template <typename T>
concept BinaryUnmarshalerFunc = requires (T m, std::span<uint8_t const> b) {
  { unmarshal_binary(m, b) } -> std::same_as<std::error_code>;
};

// TextMarshaler is the interface implemented by an object that can marshal
// itself into a textual form.
//
// MarshalText encodes the receiver into UTF-8-encoded text and returns the
// result.
template <typename T>
concept TextMarshaler = requires (T m) {
  { m.marshal_text() } -> std::same_as<std::pair<std::string, std::error_code>>;
};

template <TextMarshaler T>
std::pair<std::string, std::error_code> marshal_text(T& m) {
  return m.marshal_text();
}

template <typename T>
concept TextMarshalerFunc = requires (T m) {
  { marshal_text(m) } -> std::same_as<std::pair<std::string, std::error_code>>;
};

// TextUnmarshaler is the interface implemented by an object that can unmarshal
// a textual representation of itself.
//
// UnmarshalText must be able to decode the form generated by MarshalText.
// UnmarshalText must copy the text if it wishes to retain the text after
// returning.
template <typename T>
concept TextUnmarshaler = requires (T m) {
  { m.unmarshal_text() } -> std::same_as<std::error_code>;
};

template <TextUnmarshaler T>
std::error_code unmarshal_binary(T& m, std::string_view s) {
  return m.unmarshal_text(s);
}

template <typename T>
concept TextUnmarshalerFunc = requires (T m) {
  { unmarshal_text(m) } -> std::same_as<std::error_code>;
};

}  // namespace bongo::encoding