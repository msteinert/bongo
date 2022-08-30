// Copyright The Go Authors.

#pragma once

#include <complex>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>
#include <utility>

#include <bongo/bytes/buffer.h>
#include <bongo/fmt/detail/concepts.h>
#include <bongo/fmt/detail/fmt.h>
#include <bongo/fmt/detail/helpers.h>
#include <bongo/fmt/detail/visit_at.h>
#include <bongo/fmt/formatter.h>
#include <bongo/fmt/stringer.h>

namespace bongo::fmt::detail {

constexpr static std::string_view comma_space_string = ", ";
constexpr static std::string_view nil_angle_string = "<nil>";
constexpr static std::string_view nil_paren_string = "(nil)";
constexpr static std::string_view nil_string = "nil";
constexpr static std::string_view percent_bang_string = "%!";
constexpr static std::string_view missing_string = "(MISSING)";
constexpr static std::string_view bad_index_string = "(BADINDEX)";
constexpr static std::string_view exception_string = "(EXCEPTION=";
constexpr static std::string_view extra_string = "%!(EXTRA ";
constexpr static std::string_view bad_width_string = "%!(BADWIDTH)";
constexpr static std::string_view bad_prec_string = "%!(BADPREC)";
constexpr static std::string_view no_verb_string = "%!(NOVERB)";

class printer {
  bytes::buffer buf_;
  fmt fmt_;
  bool reordered_;
  bool good_arg_num_;

 public:
  printer() = default;

  // Implement the State concept.
  auto width() const -> std::pair<long, bool>;
  auto precision() const -> std::pair<long, bool>;
  auto flag(long b) const -> bool;
  auto write(std::span<uint8_t const> b) -> std::pair<long, std::error_code>;

  template <typename... Args>
  auto do_printf(std::string_view format, Args&&... args) -> void;

  template <typename... Args>
  auto do_print(Args&&... args) -> void;

  template <typename... Args>
  auto do_println(Args&&... args) -> void;

  auto bytes() -> std::span<uint8_t> { return buf_.bytes();}
  auto str() const -> std::string_view { return buf_.str(); }

 private:
  template <typename T>
  auto print_arg(T&& arg, rune verb) -> void;

  template <std::input_iterator It>
  auto arg_number(It it, It end, long arg_num, long num_args) -> std::tuple<long, bool, It>;

  template <typename T>
  auto bad_verb(T&& arg, rune verb) -> void;

  template <Boolean T>
  auto do_format(T&& arg, rune verb) -> void;

  template <Integer T>
  auto do_format_0x64(T&& arg, bool leading0x) -> void;

  template <Integer T>
  auto do_format(T&& arg, rune verb) -> void;

  template <Floating T>
  auto do_format(T&& arg, rune verb) -> void;

  template <Complex T>
  auto do_format(T&& arg, rune verb) -> void;

  template <String T>
  auto do_format(T&& arg, rune verb) -> void;

  template <Bytes T>
  auto do_format(T&& arg, rune verb) -> void;

  template <Pointer T>
  auto do_format(T&& arg, rune verb) -> void;

  template <Unprintable T>
  auto do_format(T&& arg, rune verb) -> void;
};

template <typename... Args>
auto printer::do_printf(std::string_view format, Args&&... args) -> void {
  size_t arg_n = sizeof... (args);
  size_t arg_num = 0;
  auto after_index = false;
  reordered_ = false;
  auto it = format.begin();
format_loop:
  while (it < format.end()) {
    good_arg_num_ = true;
    auto last_it = it;
    while (it < format.end() && *it != '%') {
      ++it;
    }
    if (it > last_it) {
      std::string_view::size_type n = it - last_it;
      buf_.write_string(std::string_view{last_it, n});
    }
    if (it == format.end()) {
      break;  // done!
    }

    ++it;  // process one verb
    fmt_.clear_flags();

    // simple_format loop
    for (; it < format.end(); ++it) {
      auto c = *it;
      switch (c) {
      case '#':
        fmt_.flags.sharp = true;
        break;
      case '0':
        fmt_.flags.zero = !fmt_.flags.minus;
        break;
      case '+':
        fmt_.flags.plus = true;
        break;
      case '-':
        fmt_.flags.minus = true;
        fmt_.flags.zero = false;
        break;
      case ' ':
        fmt_.flags.space = true;
        break;
      default:
        if ('a' <= c && c <= 'z' && arg_num < arg_n) {
          if (c == 'v') {
            // Bongo syntax
            fmt_.flags.sharp_v = fmt_.flags.sharp;
            fmt_.flags.sharp = false;
            fmt_.flags.plus_v = fmt_.flags.plus;
            fmt_.flags.plus = false;
          }
          visit_arg([this, c](auto& arg) {
            this->print_arg(arg, unicode::utf8::to_rune(c));
          }, arg_num, args...);
          ++arg_num;
          ++it;
          goto format_loop;  // continue format_loop
        }
        goto simple_format;  // break simple_format
      }
    }
simple_format:
    // Check for argument index
    std::tie(arg_num, after_index, it) = arg_number(it, format.end(), arg_num, arg_n);

    // Check for width
    if (it < format.end() && *it == '*') {
      ++it;
      std::tie(fmt_.wid, fmt_.flags.wid_present, arg_num) = int_from_arg(arg_num, args...);

      if (!fmt_.flags.wid_present) {
        buf_.write_string(bad_width_string);
      }

      if (fmt_.wid < 0) {
        fmt_.wid = -fmt_.wid;
        fmt_.flags.minus = true;
        fmt_.flags.zero = false;
      }
      after_index = false;
    } else {
      std::tie(fmt_.wid, fmt_.flags.wid_present, it) = parse_number(it, format.end());
      if (after_index && fmt_.flags.wid_present) {
        good_arg_num_ = false;
      }
    }

    // Check for precision
    if (std::next(it) < format.end() && *it == '.') {
      ++it;
      if (after_index) {
        good_arg_num_ = false;
      }
      std::tie(arg_num, after_index, it) = arg_number(it, format.end(), arg_num, arg_n);
      if (it < format.end() && *it == '*') {
        ++it;
        std::tie(fmt_.prec, fmt_.flags.prec_present, arg_num) = int_from_arg(arg_num, args...);
        if (fmt_.prec < 0) {
          fmt_.prec = 0;
          fmt_.flags.prec_present = false;
        }
        if (!fmt_.flags.prec_present) {
          buf_.write_string(bad_prec_string);
        }
        after_index = false;
      } else {
        std::tie(fmt_.prec, fmt_.flags.prec_present, it) = parse_number(it, format.end());
        if (!fmt_.flags.prec_present) {
          fmt_.prec = 0;
          fmt_.flags.prec_present = true;
        }
      }
    }

    if (!after_index) {
      std::tie(arg_num, after_index, it) = arg_number(it, format.end(), arg_num, arg_n);
    }

    if (it >= format.end()) {
      buf_.write_string(no_verb_string);
      break;
    }

    auto verb = unicode::utf8::to_rune(*it);
    auto size = 1;
    if (verb >= unicode::utf8::rune_self) {
      std::tie(verb, size) = unicode::utf8::decode(it, format.end());
    }
    it += size;

    if (verb == '%') {
      buf_.write_byte('%');
    } else if (!good_arg_num_) {
      // bad arg num
      buf_.write_string(percent_bang_string);
      buf_.write_rune(verb);
      buf_.write_string(bad_index_string);
    } else if (arg_num >= arg_n) {
      // missing arg
      buf_.write_string(percent_bang_string);
      buf_.write_rune(verb);
      buf_.write_string(missing_string);
    } else {
      if (verb == 'v') {
        // Bongo syntax
        fmt_.flags.sharp_v = fmt_.flags.sharp;
        fmt_.flags.sharp = false;
        fmt_.flags.plus_v = fmt_.flags.plus;
        fmt_.flags.plus = false;
      }
      visit_arg([this, verb](auto& arg) {
        this->print_arg(arg, verb);
      }, arg_num, args...);
      ++arg_num;
    }
  }

  if (!reordered_ && arg_num < arg_n) {
    fmt_.clear_flags();
    buf_.write_string(extra_string);
    for (auto i = arg_num; i < arg_n; ++i) {
      if (i > arg_num) {
        buf_.write_string(comma_space_string);
      }
      visit_arg([this](auto& arg) {
        if (arg == nil) {
          buf_.write_string(nil_angle_string);
        } else if constexpr (String<decltype(arg)>) {
          buf_.write_string("string");
          buf_.write_byte('=');
          fmt_.format(arg, buf_);
        } else {
          buf_.write_string(type_name(arg));
          if constexpr (requires { fmt_.format(arg, buf_); }) {
            buf_.write_byte('=');
            fmt_.format(arg, buf_);
          }
        }
      }, i, args...);
    }
    buf_.write_byte(')');
  }
}

template <typename... Args>
auto printer::do_print(Args&&... args) -> void {
  auto prev_string = false;
  for (size_t arg_num  = 0; arg_num < sizeof... (args); ++arg_num) {
    visit_at([&](auto& arg) {
      constexpr auto is_string = String<decltype(arg)>;
      if (arg_num > 0 && !is_string && !prev_string) {
        buf_.write_byte(' ');
      }
      print_arg(arg, 'v');
      prev_string = is_string;
    }, std::forward_as_tuple(args...), arg_num);
  }
}

template <typename... Args>
auto printer::do_println(Args&&... args) -> void {
  for (size_t arg_num  = 0; arg_num < sizeof... (args); ++arg_num) {
    visit_at([&](auto& arg) {
      if (arg_num > 0) {
        buf_.write_byte(' ');
      }
      print_arg(arg, 'v');
    }, std::forward_as_tuple(args...), arg_num);
  }
  buf_.write_byte('\n');
}

template <typename T>
auto printer::print_arg(T&& arg, rune verb) -> void {
  if constexpr (std::is_same_v<std::remove_cvref_t<T>, nullptr_t>) {
    fmt_.pad(buf_, nil_angle_string);
  } else {
    if (arg == nil) {
      switch (verb) {
      case 'v':
        fmt_.pad(buf_, nil_angle_string);
        break;
      default:
        break;
      }
      return;
    }

    switch (verb) {
    case 'T':
      fmt_.format(type_name(arg), buf_);
      return;
    default:
      break;
    }

    do_format(arg, verb);
  }
}

template <std::input_iterator It>
auto printer::arg_number(It it, It end, long arg_num, long num_args) -> std::tuple<long, bool, It> {
  if (it >= end || *it != '[') {
    return {arg_num, false, it};
  }
  reordered_ = true;
  auto [index, ok, new_it] = parse_arg_number(it, end);
  if (ok && 0 <= index && index < num_args) {
    return {index, true, new_it};
  }
  good_arg_num_ = false;
  return {arg_num, ok, new_it};
}

template <typename T>
auto printer::bad_verb(T&& arg, rune verb) -> void {
  buf_.write_string(percent_bang_string);
  buf_.write_rune(verb);
  buf_.write_byte('(');
  buf_.write_string(type_name(arg));
  if constexpr (requires { fmt_.format(arg, buf_); }) {
    buf_.write_byte('=');
    fmt_.format(arg, buf_);
  }
  buf_.write_byte(')');
}

template <Boolean T>
auto printer::do_format(T&& arg, rune verb) -> void {
  switch (verb) {
  case 't': case 'v':
    fmt_.format(arg, buf_);
    break;
  default:
    bad_verb(arg, verb);
    break;
  }
}

template <Integer T>
auto printer::do_format_0x64(T&& arg, bool leading0x) -> void {
  auto sharp = fmt_.flags.sharp;
  fmt_.flags.sharp = leading0x;
  fmt_.format(arg, 16, 'v', ldigits, buf_);
  fmt_.flags.sharp = sharp;
}

template <Integer T>
auto printer::do_format(T&& arg, rune verb) -> void {
  switch (verb) {
  case 'v':
    if (fmt_.flags.sharp_v && !std::is_signed_v<std::remove_cvref_t<T>>) {
      do_format_0x64(arg, true);
    } else {
      fmt_.format(arg, 10, verb, ldigits, buf_);
    }
    break;
  case 'd':
    fmt_.format(arg, 10, verb, ldigits, buf_);
    break;
  case 'b':
    fmt_.format(arg, 2, verb, ldigits, buf_);
    break;
  case 'o': case 'O':
    fmt_.format(arg, 8, verb, ldigits, buf_);
    break;
  case 'x':
    fmt_.format(arg, 16, verb, ldigits, buf_);
    break;
  case 'X':
    fmt_.format(arg, 16, verb, udigits, buf_);
    break;
  case 'c':
    fmt_.format_c(arg, buf_);
    break;
  case 'q':
    fmt_.format_qc(arg, buf_);
    break;
  case 'U':
    fmt_.format_unicode(arg, buf_);
    break;
  default:
    bad_verb(arg, verb);
    break;
  }
}

template <Floating T>
auto printer::do_format(T&& arg, rune verb) -> void {
  switch (verb) {
  case 'v':
    fmt_.format(arg, 'g', -1, buf_);
    break;
  case 'b': case 'g': case 'G': case 'x': case 'X':
    fmt_.format(arg, verb, -1, buf_);
    break;
  case 'f': case 'e': case 'E':
    fmt_.format(arg, verb, 6, buf_);
    break;
  case 'F':
    fmt_.format(arg, 'f', 6, buf_);
    break;
  default:
    bad_verb(arg, verb);
    break;
  }
}

template <Complex T>
auto printer::do_format(T&& arg, rune verb) -> void {
  long old_plus;
  switch (verb) {
  case 'v': case 'b': case 'g': case 'G': case 'x': case 'X': case 'f': case 'F': case 'e': case 'E':
    old_plus = fmt_.flags.plus;
    buf_.write_byte('(');
    do_format(arg.real(), verb);
    // Imaginary part always has a sign.
    fmt_.flags.plus = true;
    do_format(arg.imag(), verb);
    buf_.write_string("i)");
    fmt_.flags.plus = old_plus;
    break;
  default:
    bad_verb(arg, verb);
    break;
  }
}

template <String T>
auto printer::do_format(T&& arg, rune verb) -> void {
  switch (verb) {
  case 'v':
    if (fmt_.flags.sharp_v) {
      fmt_.format_q(std::string_view{arg}, buf_);
    } else {
      fmt_.format(std::string_view{arg}, buf_);
    }
    break;
  case 's':
    fmt_.format(std::string_view{arg}, buf_);
    break;
  case 'x':
    fmt_.format(std::string_view{arg}, ldigits, buf_);
    break;
  case 'X':
    fmt_.format(std::string_view{arg}, udigits, buf_);
    break;
  case 'q':
    fmt_.format_q(std::string_view{arg}, buf_);
    break;
  default:
    bad_verb(arg, verb);
    break;
  }
}

template <Bytes T>
auto printer::do_format(T&& arg, rune verb) -> void {
  switch (verb) {
  case 'v': case 'd':
    if (fmt_.flags.sharp_v) {
      buf_.write_string(detail::type_name(arg));
      if (arg == nil) {
        buf_.write_string(nil_paren_string);
        return;
      }
      buf_.write_byte('{');
      for (auto it = arg.begin(); it < arg.end(); ++it) {
        if (it != arg.begin()) {
          buf_.write_string(comma_space_string);
        }
        do_format_0x64(*it, true);
      }
      buf_.write_byte('}');
    } else {
      buf_.write_byte('[');
      for (auto it = arg.begin(); it < arg.end(); ++it) {
        if (it != arg.begin()) {
          buf_.write_byte(' ');
        }
        fmt_.format(*it, 10, verb, ldigits, buf_);
      }
      buf_.write_byte(']');
    }
    break;
  case 's':
    fmt_.format(bytes::to_string(arg), buf_);
    break;
  case 'x':
    fmt_.format(arg, ldigits, buf_);
    break;
  case 'X':
    fmt_.format(arg, udigits, buf_);
    break;
  case 'q':
    fmt_.format_q(bytes::to_string(arg), buf_);
    break;
  default:
    bad_verb(arg, verb);
    break;
  }
}

template <Pointer T>
auto printer::do_format(T&& arg, rune verb) -> void {
  switch (verb) {
  case 'v':
    if (fmt_.flags.sharp_v) {
      buf_.write_byte('(');
      buf_.write_string(type_name(arg));
      buf_.write_string(")(");
      if (arg == nullptr) {
        buf_.write_string(nil_string);
      } else {
        do_format_0x64(reinterpret_cast<uintptr_t>(arg), true);
      }
      buf_.write_byte(')');
    } else {
      if (arg == nullptr) {
        fmt_.pad(buf_, nil_angle_string);
      } else {
        do_format_0x64(reinterpret_cast<uintptr_t>(arg), !fmt_.flags.sharp);
      }
    }
    break;
  case 'p':
    do_format_0x64(reinterpret_cast<uintptr_t>(arg), !fmt_.flags.sharp);
    break;
  case 'b': case 'o': case 'd': case 'x': case 'X':
    do_format(reinterpret_cast<uintptr_t>(arg), verb);
    break;
  default:
    bad_verb(arg, verb);
    break;
  }
}

template <Unprintable T>
auto printer::do_format(T&& arg, rune verb) -> void {
  if constexpr (FormatterFunc<T, printer>) {
    format(const_cast<T&>(arg), *this, verb);
    return;
  }

  if constexpr (BongoStringerFunc<T>) {
    if (fmt_.flags.sharp_v) {
      fmt_.format(to_bongo_string(arg), buf_);
      return;
    }
  }

  if constexpr (StringerFunc<T>) {
    do_format(to_string(arg), verb);
    return;
  }

  if constexpr (Iterable<T>) {
    if (fmt_.flags.sharp_v) {
      buf_.write_string(detail::type_name(arg));
      if (arg == nil) {
        buf_.write_string(nil_paren_string);
        return;
      }
      buf_.write_byte('{');
      for (auto it = std::begin(arg); it != std::end(arg); ++it) {
        if (it != arg.begin()) {
          buf_.write_string(comma_space_string);
        }
        do_format(*it, verb);
      }
      buf_.write_byte('}');
    } else {
      buf_.write_byte('[');
      for (auto it = std::begin(arg); it != std::end(arg); ++it) {
        if (it != arg.begin()) {
          buf_.write_byte(' ');
        }
        do_format(*it, verb);
      }
      buf_.write_byte(']');
    }
    return;
  }

  if constexpr (Pair<T>) {
    if (fmt_.flags.sharp_v) {
      buf_.write_string(detail::type_name(arg));
      buf_.write_byte('{');
      do_format(arg.first, verb);
      buf_.write_string(comma_space_string);
      do_format(arg.second, verb);
      buf_.write_byte('}');
    } else {
      do_format(arg.first, verb);
      buf_.write_byte(':');
      do_format(arg.second, verb);
    }
    return;
  }

  if constexpr (Tuple<T>) {
    constexpr auto size = std::tuple_size<std::remove_cvref_t<T>>::value;
    if (fmt_.flags.sharp_v) {
      buf_.write_string(detail::type_name(arg));
      buf_.write_byte('{');
      for (size_t i = 0; i < size; ++i) {
        if (i != 0) {
          buf_.write_string(comma_space_string);
        }
        visit_at([this, verb](auto& arg) {
          this->do_format(arg, verb);
        }, arg, i);
      }
      buf_.write_byte('}');
    } else {
      buf_.write_byte('[');
      for (size_t i = 0; i < size; ++i) {
        if (i != 0) {
          buf_.write_byte(' ');
        }
        visit_at([this, verb](auto& arg) {
          this->do_format(arg, verb);
        }, arg, i);
      }
      buf_.write_byte(']');
    }
    return;
  }

  if constexpr (ErrorCodeEnum<T>) {
    using std::make_error_code;
    switch (verb) {
    case 'v': case 's': case 'q':
      do_format(make_error_code(arg).message(), verb);
      break;
    default:
      do_format(static_cast<std::underlying_type_t<T>>(arg), verb);
      break;
    }
    return;
  }

  if constexpr (ErrorConditionEnum<T>) {
    using std::make_error_condition;
    switch (verb) {
    case 'v': case 's': case 'q':
      do_format(make_error_condition(arg).message(), verb);
      break;
    default:
      do_format(static_cast<std::underlying_type_t<T>>(arg), verb);
      break;
    }
    return;
  }

  if constexpr (Error<T>) {
    switch (verb) {
    case 'v': case 's': case 'q':
      do_format(arg.message(), verb);
      break;
    default:
      do_format(arg.value(), verb);
      break;
    }
    return;
  }

  if constexpr (Exception<T>) {
    do_format(arg.what(), verb);
    return;
  }

  if (arg == nil) {
    buf_.write_string(nil_angle_string);
  } else {
    buf_.write_byte('?');
    buf_.write_string(type_name(arg));
    buf_.write_byte('?');
  }
}

}  // namespace bongo::fmt::detail
