// Copyright The Go Authors.

#pragma once

#include <iterator>

#include <bongo/unicode/utf8/iterator.h>

namespace bongo::unicode::utf8 {

template <typename T>
constexpr auto begin(T const& v)
  noexcept(noexcept(std::begin(v)) && noexcept(std::end(v)))
  -> unicode::utf8::iterator<typename T::const_iterator>
{ return unicode::utf8::iterator{std::begin(v), std::begin(v), std::end(v)}; }

template <typename T>
constexpr auto end(T const& v)
  noexcept(noexcept(std::begin(v)) && noexcept(std::end(v)))
  -> unicode::utf8::iterator<typename T::const_iterator>
{ return unicode::utf8::iterator{std::end(v), std::begin(v), std::end(v)}; }

template <typename T>
class range {
 public:
  using const_iterator = unicode::utf8::iterator<typename T::const_iterator>;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;

 private:
  typename T::const_iterator begin_;
  typename T::const_iterator end_;

 public:
  constexpr range(typename T::const_iterator begin, typename T::const_iterator end)
    noexcept(std::is_nothrow_copy_constructible_v<T>)
      : begin_{begin}
      , end_{end}
  { static_assert(sizeof (typename T::value_type) == 1); }

  constexpr range(T const& v)
    noexcept(noexcept(std::begin(v)) && noexcept(std::end(v)))
      : range{std::begin(v), std::end(v)} {}

  constexpr iterator begin() const noexcept
  { return unicode::utf8::iterator{begin_, begin_, end_}; }

  constexpr iterator end() const noexcept
  { return unicode::utf8::iterator{end_, begin_, end_}; }
};

}  // namesapce bongo::unicode::utf8
