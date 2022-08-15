// Copyright The Go Authors.

#pragma once

#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace bongo::runtime {

struct nil_t {
  constexpr auto operator==(nil_t const&) const noexcept -> bool { return true; }
  constexpr auto operator==(nullptr_t const&) const noexcept -> bool { return true; }
  auto operator==(std::error_code const& err) const noexcept -> bool { return err == std::error_code{}; }
  auto operator==(std::string const& s) const noexcept { return s.size() == 0; }
  constexpr auto operator==(std::string_view const& s) const noexcept -> bool { return s.size() == 0; }

  template <typename T>
  constexpr auto operator==(std::span<T> const& s) const noexcept -> bool { return s.size() == 0; }

  constexpr operator bool() const noexcept { return false; }
  constexpr operator void*() const noexcept { return nullptr; }
  operator std::error_code() const noexcept { return std::error_code{}; }
  operator std::string() const noexcept { return std::string{}; }
  constexpr operator std::string_view() const noexcept { return std::string_view{}; }

  template <typename T>
  constexpr operator std::span<T>() const noexcept { return std::span<T>{}; }

  template <typename T>
  constexpr auto operator==(T const&) const noexcept -> bool { return false; }
};

}  // namespace bongo::runtime
