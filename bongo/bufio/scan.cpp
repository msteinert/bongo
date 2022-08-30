// Copyright The Go Authors.

#include <optional>
#include <span>
#include <string_view>
#include <system_error>
#include <tuple>

#include <bongo/bongo.h>
#include <bongo/bufio/scan.h>
#include <bongo/bytes.h>
#include <bongo/unicode/utf8.h>

namespace bongo::bufio {
namespace {

static constexpr std::string_view error_rune = "\ufffd";

auto drop_cr(std::span<uint8_t const> data) -> std::span<uint8_t const> {
  if (data.size() > 0 && data.back() == '\r') {
    return data.subspan(0, data.size()-1);
  }
  return data;
}

auto is_space(rune r) -> bool {
  if (r <= 0x00ff) {
    switch (r) {
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
      return true;
    case 0x0085:
    case 0x00a0:
      return true;
    }
  }
  if (0x2000 <= r && r <= 0x200a) {
    return true;
  }
  switch (r) {
  case 0x1680:
  case 0x2028:
  case 0x2029:
  case 0x202f:
  case 0x205f:
  case 0x3000:
    return true;
  }
  return false;
}

}  // namespace

auto scan_bytes(std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
  if (at_eof && data.size() == 0) {
    return {0, std::nullopt, nil};
  }
  return {1, data.subspan(0, 1), nil};
}

auto scan_runes(std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
  namespace utf8 = unicode::utf8;
  if (at_eof && data.size() == 0) {
    return {0, std::nullopt, nil};
  }
  if (data.front() < utf8::rune_self) {
    return {1, data.subspan(0, 1), nil};
  }
  auto [_, width] = utf8::decode(data);
  if (width > 1) {
    return {width, data.subspan(0, width), nil};
  }
  if (!at_eof && !utf8::full_rune(data)) {
    return {0, std::nullopt, nil};
  }
  return {1, bytes::to_bytes(error_rune), nil};
}

auto scan_lines(std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
  if (at_eof && data.size() == 0) {
    return {0, std::nullopt, nil};
  }
  if (auto i = bytes::index(data, '\n'); i != bytes::npos) {
    return {i + 1, drop_cr(data.subspan(0, i)), nil};
  }
  if (at_eof) {
    return {data.size(), drop_cr(data), nil};
  }
  return {0, std::nullopt, nil};
}

auto scan_words(std::span<uint8_t const> data, bool at_eof) -> std::tuple<long, std::optional<std::span<uint8_t const>>, std::error_code> {
  namespace utf8 = unicode::utf8;
  auto start = 0;
  for (long width = 0; start < static_cast<long>(data.size()); start += width) {
    rune r;
    std::tie(r, width) = utf8::decode(data.subspan(start));
    if (!is_space(r)) {
      break;
    }
  }
  for (long width = 0, i = start; i < static_cast<long>(data.size()); i += width) {
    rune r;
    std::tie(r, width) = utf8::decode(data.subspan(i));
    if (is_space(r)) {
      return {i + width, data.subspan(start, i-start), nil};
    }
  }
  if (at_eof && static_cast<long>(data.size()) > start) {
    return {data.size(), data.subspan(start), nil};
  }
  return {start, std::nullopt, nil};
}

}  // namespace bongo::bufio
