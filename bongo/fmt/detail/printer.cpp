// Copyright The Go Authors.

#include <system_error>
#include <span>
#include <utility>

#include <bongo/fmt/detail/printer.h>

namespace bongo::fmt::detail {

auto printer::width() const -> std::pair<long, bool> {
  return {fmt_.wid, fmt_.flags.wid_present};
}

auto printer::precision() const -> std::pair<long, bool> {
  return {fmt_.prec, fmt_.flags.prec_present};
}

auto printer::flag(long b) const -> bool {
  switch (b) {
  case '-':
    return fmt_.flags.minus;
  case '+':
    return fmt_.flags.plus || fmt_.flags.plus_v;
  case '#':
    return fmt_.flags.sharp || fmt_.flags.sharp_v;
  case ' ':
    return fmt_.flags.space;
  case '0':
    return fmt_.flags.zero;
  default:
    return false;
  }
}

auto printer::write(std::span<uint8_t const> b) -> std::pair<long, std::error_code> {
  buf_.write(b);
  return {b.size(), nil};
}

}  // namespace bongo::fmt::detail
