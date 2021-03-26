// Copyright Exegy, Inc.
// Copyright The Go Authors.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>

namespace bongo {

template <typename T> class chan;

/**
 * Input to the select function.
 *
 * Use one of the factory functions to create a case:
 *
 * - send_select_case
 * - recv_select_case
 * - default_select_case
 */
struct select_case {
  detail::select_direction direction;
  detail::chan* chan = nullptr;
  detail::select_value value = nullptr;
};

/**
 * Create a send case for to select.
 *
 * If this case is trigger the value \p v will be moved.
 *
 * \param c The channel participating in the send
 * \param v The value to send
 */
template <typename T>
select_case send_select_case(chan<T>& c, T&& v) {
  return select_case{detail::select_send, std::addressof(c), std::addressof(v)};
}

/**
 * Create a send case for to select.
 *
 * If this case is trigger the value \p v will be moved.
 *
 * \param c The channel participating in the send (can be nullptr)
 * \param v The value to send
 */
template <typename T>
select_case send_select_case(chan<T>* c, T&& v) {
  return select_case{detail::select_send, c, std::addressof(v)};
}

/**
 * Create a receive case for to select.
 *
 * \param c The channel participating in the receive
 * \param v Storage for receiving the value
 *
 * \returns A receive select case.
 */
template <typename T>
select_case recv_select_case(chan<T>& c, std::optional<T>& v) {
  return select_case{detail::select_recv, std::addressof(c), std::addressof(v)};
}

/**
 * Create a receive case for to select.
 *
 * \param c The channel participating in the receive (can be nullptr)
 * \param v Storage for receiving the value
 *
 * \returns A receive select case.
 */
template <typename T>
select_case recv_select_case(chan<T>* c, std::optional<T>& v) {
  return select_case{detail::select_recv, c, std::addressof(v)};
}

/**
 * Create a default case case for to select.
 *
 * \returns A default select case.
 */
inline select_case default_select_case() {
  return select_case{detail::select_default, nullptr, nullptr};
}

/**
 * Execute a select operation.
 *
 * This select implementation is similar to reflect.Select. The input is an
 * array of select cases. The output reflects the index that triggered the
 * select. The default case is triggered if no channels are ready.
 *
 * This function is the base implementation. One of the other overloads is most
 * likely more appropriate for user code.
 *
 * - https://golang.org/ref/spec#Select_statements
 * - https://tour.golang.org/concurrency/5
 * - https://golang.org/pkg/reflect/#Select
 *
 * @param cases An array of select cases
 * @param n The length of \p cases
 *
 * @returns The index in \p cases that triggered the select.
 */
size_t select(select_case const* cases, size_t n);

/**
 * Execute a select operation.
 *
 * Use this overload if you wish to dynamically change the select set at runtime.
 *
 * @param cases A vector of select cases
 *
 * @returns The index in \p cases that triggered the select.
 */
inline size_t select(std::vector<select_case> const& cases) {
  return select(cases.data(), cases.size());
}

/**
 * Execute a select operation.
 *
 * Use this overload if the number of cases is known at compile time.
 *
 * @param cases An array of select cases
 *
 * @returns The index in \p cases that triggered the select.
 */
template <size_t N>
size_t select(const select_case (&cases)[N]) {
  return select(cases, N);
}

/**
 * Execute a select operation.
 *
 * Use this overload if the number of cases is known at compile time.
 *
 * @param args A parameter pack of select cases
 *
 * @returns The index in \p cases that triggered the select.
 */
template <typename... Args>
std::enable_if_t<
  std::conjunction_v<std::is_same<select_case, Args>...>,
size_t> select(Args&&... args) {
  std::array<select_case, sizeof...(Args)> cases = {std::forward<Args>(args)...};
  return select(cases.data(), cases.size());
}

}  // namespace bongo
