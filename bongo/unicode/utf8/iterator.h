// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <iterator>
#include <tuple>

#include <bongo/rune.h>
#include <bongo/unicode/utf8.h>

namespace bongo::unicode::utf8 {

template <typename Iterator>
class iterator {
 public:
  using iterator_category = std::input_iterator_tag;
  using value_type = std::pair<size_t, rune>;
  using const_pointer = value_type const*;
  using pointer = const_pointer;
  using const_reference = value_type const&;
  using reference = const_reference;
  using difference_type = ssize_t;

 private:
  Iterator it_;       // Current location
  Iterator begin_;    // Start of the range
  Iterator end_;      // End of the range
  value_type value_;  // The current index/rune
  size_t size_;       // Size of the current rune

  // Note: Since this is a so-called stashing iterator it cannot be used with
  // std::reverse_iterator.

 public:
  constexpr iterator(Iterator it, Iterator begin, Iterator end) noexcept
      : it_{it}
      , begin_{begin}
      , end_{end} {
    static_assert(sizeof (typename std::iterator_traits<Iterator>::value_type) == 1);
    if (it_ != end_) {
      auto [value, size] = unicode::utf8::decode(it_, end_);
      value_ = {0, value};
      size_ = size;
    } else {
      value_ = {std::distance(begin_, end_), unicode::utf8::rune_error};
      size_ = 0;
    }
  }

  constexpr pointer operator->() const noexcept { return &value_; }
  constexpr reference operator*() const noexcept { return value_; }
  const Iterator& base() const noexcept { return it_; }

  constexpr iterator& operator++() noexcept {
    it_ = std::next(it_, size_);
    auto [value, size] = unicode::utf8::decode(it_, end_);
    value_ = {std::get<0>(value_)+size_, value};
    size_ = size;
    return *this;
  }

  constexpr iterator operator++(int) noexcept {
    auto cur = *this;
    operator++();
    return cur;
  }

  constexpr iterator& operator--() noexcept(noexcept(std::prev(it_))) {
    auto [value, size] = unicode::utf8::decode_last(begin_, it_);
    size_ = size;
    value_ = {std::get<0>(value_)-size_, value};
    it_ = std::prev(it_, size_);
    return *this;
  }

  constexpr iterator operator--(int) noexcept {
    auto cur = *this;
    operator--();
    return cur;
  }
};

template <typename IteratorL, typename IteratorR>
constexpr bool operator==(iterator<IteratorL> const& lhs,
                          iterator<IteratorR> const& rhs) noexcept
{ return lhs.base() == rhs.base(); }

template <typename Iterator>
constexpr bool operator==(iterator<Iterator> const& lhs,
                          iterator<Iterator> const& rhs) noexcept
{ return lhs.base() == rhs.base(); }

template <typename IteratorL, typename IteratorR>
constexpr bool operator!=(iterator<IteratorL> const& lhs,
                          iterator<IteratorR> const& rhs) noexcept
{ return lhs.base() != rhs.base(); }

template <typename Iterator>
constexpr bool operator!=(iterator<Iterator> const& lhs,
                          iterator<Iterator> const& rhs) noexcept
{ return lhs.base() != rhs.base(); }

template <typename IteratorL, typename IteratorR>
constexpr bool operator<(iterator<IteratorL> const& lhs,
                         iterator<IteratorR> const& rhs) noexcept
{ return lhs.base() < rhs.base(); }

template <typename Iterator>
constexpr bool operator<(iterator<Iterator> const& lhs,
                         iterator<Iterator> const& rhs) noexcept
{ return lhs.base() < rhs.base(); }

template <typename IteratorL, typename IteratorR>
constexpr bool operator>(iterator<IteratorL> const& lhs,
                         iterator<IteratorR> const& rhs) noexcept
{ return lhs.base() > rhs.base(); }

template <typename Iterator>
constexpr bool operator>(iterator<Iterator> const& lhs,
                         iterator<Iterator> const& rhs) noexcept
{ return lhs.base() > rhs.base(); }

template <typename IteratorL, typename IteratorR>
constexpr bool operator<=(iterator<IteratorL> const& lhs,
                          iterator<IteratorR> const& rhs) noexcept
{ return lhs.base() <= rhs.base(); }

template <typename Iterator>
constexpr bool operator<=(iterator<Iterator> const& lhs,
                          iterator<Iterator> const& rhs) noexcept
{ return lhs.base() <= rhs.base(); }

template <typename IteratorL, typename IteratorR>
constexpr bool operator>=(iterator<IteratorL> const& lhs,
                          iterator<IteratorR> const& rhs) noexcept
{ return lhs.base() >= rhs.base(); }

template <typename Iterator>
constexpr bool operator>=(iterator<Iterator> const& lhs,
                          iterator<Iterator> const& rhs) noexcept
{ return lhs.base() >= rhs.base(); }

}  // namesapce bongo::unicode::utf8
