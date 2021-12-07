// Copyright The Go Authors.

#pragma once

#include <type_traits>
#include <utility>

#include <bongo/math/bits.h>
#include <bongo/strconv/detail/decimal.h>
#include <bongo/strconv/detail/float_info.h>
#include <bongo/strconv/detail/itoa.h>
#include <bongo/strconv/detail/powers_of_ten.h>

namespace bongo::strconv::detail {

inline std::pair<uint32_t, uint32_t> div_mod_1e9(uint64_t v) {
  uint64_t hi = 0;
  std::tie(hi, std::ignore) = math::bits::mul64(v>>1, 0x89705f4136b4a598);
  auto q = hi >> 28;
  return {static_cast<uint32_t>(q), static_cast<uint32_t>(v - static_cast<uint64_t>(q*1e9))};
}

constexpr long mul_by_log2_log10(long v) {
  return (v * 78913) >> 18;
}

constexpr long mul_by_log10_log2(long v) {
  return (v * 108853) >> 15;
}

constexpr std::tuple<uint32_t, long, bool> mul_64bit_pow10(uint32_t m, uint32_t e2, long q) {
  if (q == 0) {
    return {m << 6, e2 - 6, true};
  }
  auto pow = std::get<1>(detailed_powers_of_ten[q-detailed_powers_of_ten_min_exp10]);
  if (q < 0) {
    ++pow;
  }
  auto [hi, lo] = math::bits::mul64(static_cast<uint64_t>(m), pow);
  e2 += mul_by_log10_log2(q) - 63 + 57;
  return {static_cast<uint32_t>((hi<<7) | (lo>>57)), e2, (lo<<7) == 0};
}

constexpr std::tuple<uint64_t, long, bool> mul_128bit_pow10(uint64_t m, long e2, long q) {
  if (q == 0) {
    return {m << 8, e2 - 8, true};
  }
  auto pow = detailed_powers_of_ten[q-detailed_powers_of_ten_min_exp10];
  if (q < 0) {
    std::get<0>(pow) += 1;
  }
  e2 += mul_by_log10_log2(q) - 127 + 119;
  auto [l1, l0] = math::bits::mul64(m, std::get<0>(pow));
  auto [h1, h0] = math::bits::mul64(m, std::get<1>(pow));
  auto [mid, carry] = math::bits::add64(l1, h0, 0);
  h1 += carry;
  return {(h1<<9) | (mid>>55), e2, (mid<<9) == 0 && l0 == 0};
}

constexpr bool divisible_by_power5(uint64_t m, long k) {
  if (m == 0) {
    return true;
  }
  for (auto i = 0; i < k; ++i) {
    if (m%5 != 0) {
      return false;
    }
    m /= 5;
  }
  return true;
}

template <typename T>
constexpr std::tuple<uint64_t, uint64_t, uint64_t, long> compute_bounds(uint64_t mant, long exp) {
  if (mant != (1ul<<float_info<T>::mantissa_bits())
      || exp == float_info<T>::bias()+1-static_cast<long>(float_info<T>::mantissa_bits())) {
    return {2*mant-1, 2*mant, 2*mant+1, exp-1};
  } else {
    return {4*mant-1, 4*mant, 4*mant+2, exp-2};
  }
}

constexpr void ryu_digits32(decimal& d, uint32_t lower, uint32_t central, uint32_t upper, bool c0, bool cup, long end_index) {
  if (upper == 0) {
    d.dp = end_index + 1;
    return;
  }
  auto trimmed = 0;
  auto cnext_digit = 0;
  while (upper > 0) {
    auto l = (lower + 9) / 10;
    auto c = central / 10;
    auto cdigit = central % 10;
    auto u = upper / 10;
    if (l > u) {
      break;
    }
    if (l == c+1 && c < u) {
      ++c;
      cdigit = 0;
      cup = false;
    }
    ++trimmed;
    c0 = c0 && cnext_digit == 0;
    cnext_digit = static_cast<long>(cdigit);
    lower = l;
    central = c;
    upper = u;
  }
  if (trimmed > 0) {
    cup = cnext_digit > 5 ||
      (cnext_digit == 5 && !c0) ||
      (cnext_digit == 5 && c0 && (central&1) == 1);
  }
  if (central < upper && cup) {
    ++central;
  }
  end_index -= trimmed;
  auto v = central;
  auto n = end_index;
  while (n > d.nd) {
    auto v1 = v/100;
    auto v2 = v%100;
    d.d[  n] = small_string[2*v2+1];
    d.d[n-1] = small_string[2*v2+0];
    n -= 2;
    v = v1;
  }
  if (n == d.nd) {
    d.d[n] = static_cast<char>(v + '0');
  }
  d.nd = end_index + 1;
  d.dp = d.nd + trimmed;
}

inline void ryu_digits(decimal& d, uint64_t lower, uint64_t central, uint64_t upper, bool c0, bool cup) {
  auto [lhi, llo] = div_mod_1e9(lower);
  auto [chi, clo] = div_mod_1e9(central);
  auto [uhi, ulo] = div_mod_1e9(upper);
  if (uhi == 0) {
    // only low digits (for denormals)
    ryu_digits32(d, llo, clo, ulo, c0, cup, 8);
  } else if (lhi < uhi) {
    // truncate 9 digits at once.
    if (llo != 0) {
      ++lhi;
    }
    c0 = (c0 && clo) == 0;
    cup = (clo > 5e8) || (clo == 5e8 && cup);
    ryu_digits32(d, lhi, chi, uhi, c0, cup, 8);
    d.dp += 9;
  } else {
    d.nd = 0;
    // emit high part
    auto n = 9lu;
    for (auto v = chi; v > 0;) {
      auto v1 = v/10;
      auto v2 = v%10;
      v = v1;
      d.d[--n] = static_cast<char>(v2 + '0');
    }
    std::copy(std::next(d.d.begin(), n), std::next(d.d.begin(), d.d.size()), d.d.begin());
    d.nd = static_cast<long>(9 - n);
    // emit low part
    ryu_digits32(d, llo, clo, ulo, c0, cup, d.nd+8);
  }
  // trim trailing zeros
  while (d.nd > 0 && d.d[d.nd-1] == '0') {
    --d.nd;
  }
  // trim initial zeros
  while (d.nd > 0 && d.d[0] == '0') {
    --d.nd;
    --d.dp;
    std::copy(std::next(d.d.begin()), std::next(d.d.begin(), d.d.size()), d.d.begin());
  }
}

constexpr void ryu_format_decimal(decimal& d, uint64_t m, bool trunc, bool round_up, long prec) {
  auto max = powers_of_ten[prec];
  auto trimmed = 0;
  while (m >= max) {
    auto a = m / 10;
    auto b = m % 10;
    m = a;
    ++trimmed;
    if (b > 5) {
      round_up = true;
    } else if (b < 5) {
      round_up = false;
    } else {
      round_up = trunc || (m&1) == 1;
    }
    if (b != 0) {
      trunc = true;
    }
  }
  if (round_up) {
    ++m;
  }
  if (m >= max) {
    m /= 10;
    ++trimmed;
  }
  auto n = static_cast<long unsigned>(prec);
  d.nd = static_cast<long>(prec);
  auto v = m;
  while (v >= 100) {
    uint64_t v1 = 0, v2 = 0;
    if (v>>32 == 0) {
      v1 = static_cast<uint64_t>(static_cast<uint32_t>(v)/100);
      v2 = static_cast<uint64_t>(static_cast<uint32_t>(v)%100);
    } else {
      v1 = v / 100;
      v2 = v % 100;
    }
    n -= 2;
    d.d[n+1] = small_string[2*v2+1];
    d.d[n+0] = small_string[2*v2+0];
    v = v1;
  }
  if (v > 0) {
    d.d[--n] = small_string[2*v+1];
  }
  if (v >= 10) {
    d.d[--n] = small_string[2*v];
  }
  while (d.d[d.nd-1] == '0') {;
    --d.nd;
    ++trimmed;
  }
  d.dp = d.nd + trimmed;
}

constexpr decimal ryu_ftoa_fixed32(uint32_t mant, long exp, long prec) {
  auto d = decimal{};
  if (mant == 0) {
    d.nd = 0;
    d.dp = 0;
    return d;
  }
  auto e2 = exp;
  if (auto b = math::bits::len(mant); b < 25) {
    mant <<= static_cast<long unsigned>(25 - b);
    e2 += static_cast<long>(b) - 25;
  }
  auto q = -mul_by_log2_log10(e2+24) + prec - 1;
  auto exact = q <= 27 && q >= 0;
  auto [di, dexp2, d0] = mul_64bit_pow10(mant, e2, q);
  if (q < 0 && q >= -10 && divisible_by_power5(static_cast<uint64_t>(mant), -q)) {
    exact = true;
    d0 = true;
  }
  auto extra = static_cast<long unsigned>(-dexp2);
  auto extra_mask = static_cast<uint32_t>((1lu<<extra) - 1);
  auto dfrac = di & extra_mask;
  di = di >> extra;
  auto round_up = false;
  if (exact) {
    round_up = dfrac > (1lu<<(extra-1)) ||
      (dfrac == 1lu<<(extra-1) && !d0) ||
      (dfrac == 1lu<<(extra-1) && d0 && (di&1) == 1);
  } else {
    round_up = dfrac>>(extra-1) == 1;
  }
  if (dfrac != 0) {
    d0 = false;
  }
  ryu_format_decimal(d, di, !d0, round_up, prec);
  d.dp -= q;
  return d;
}

constexpr decimal ryu_ftoa_fixed64(uint64_t mant, long exp, long prec) {
  auto d = decimal{};
  if (mant == 0) {
    d.nd = 0;
    d.dp = 0;
    return d;
  }
  auto e2 = exp;
  if (auto b = math::bits::len(mant); b < 55) {
    mant = mant << static_cast<long unsigned>(55-b);
    e2 += static_cast<long>(b) - 55;
  }
  auto q = -mul_by_log2_log10(e2+54) + prec - 1;
  auto exact = q <= 55 && q >= 0;
  auto [di, dexp2, d0] = mul_128bit_pow10(mant, e2, q);
  if (q < 0 && q >= -22 && divisible_by_power5(mant, -q)) {
    exact = true;
    d0 = true;
  }
  auto extra = static_cast<long unsigned>(-dexp2);
  auto extra_mask = static_cast<uint64_t>((1lu<<extra) - 1);
  auto dfrac = di & extra_mask;
  di = di >> extra;
  auto round_up = false;
  if (exact) {
    round_up = dfrac > (1lu<<(extra-1)) ||
      (dfrac == 1lu<<(extra-1) && !d0) ||
      (dfrac == 1lu<<(extra-1) && d0 && (di&1) == 1);
  } else {
    round_up = dfrac>>(extra-1) == 1;
  }
  if (dfrac != 0) {
    d0 = false;
  }
  ryu_format_decimal(d, di, !d0, round_up, prec);
  d.dp -= q;
  return d;
}

template <typename T>
constexpr decimal ryu_ftoa_shortest(uint64_t mant, long exp) {
  auto d = decimal{};
  if (mant == 0) {
    d.nd = 0;
    d.dp = 0;
    return d;
  }
  if (exp <= 0 && math::bits::trailing_zeros(mant) >= -exp) {
    mant >>= static_cast<long unsigned>(-exp);
    ryu_digits(d, mant, mant, mant, true, false);
    return d;
  }
  auto [ml, mc, mu, e2] = compute_bounds<T>(mant, exp);
  if (e2 == 0) {
    ryu_digits(d, ml, mc, mu, true, false);
    return d;
  }
  auto q = mul_by_log2_log10(-e2) + 1;
  uint64_t dl = 0, dc = 0, du = 0;
  bool dl0 = false, dc0 = false, du0 = false;
  if constexpr (std::is_same_v<float, std::remove_cv_t<T>>) {
    uint32_t dl32 = 0, dc32 = 0, du32 = 0;
    std::tie(dl32, std::ignore, dl0) = mul_64bit_pow10(static_cast<uint32_t>(ml), e2, q);
    std::tie(dc32, std::ignore, dc0) = mul_64bit_pow10(static_cast<uint32_t>(mc), e2, q);
    std::tie(du32, e2,          du0) = mul_64bit_pow10(static_cast<uint32_t>(mu), e2, q);
    dl = static_cast<uint64_t>(dl32);
    dc = static_cast<uint64_t>(dc32);
    du = static_cast<uint64_t>(du32);
  } else {
    std::tie(dl, std::ignore, dl0) = mul_128bit_pow10(ml, e2, q);
    std::tie(dc, std::ignore, dc0) = mul_128bit_pow10(mc, e2, q);
    std::tie(du, e2,          du0) = mul_128bit_pow10(mu, e2, q);
  }
  if (q > 55) {
    dl0 = dc0 = du0 = false;
  }
  if (q < 0 && q >= -24) {
    if (divisible_by_power5(ml, -q)) {
      dl0 = true;
    }
    if (divisible_by_power5(mc, -q)) {
      dc0 = true;
    }
    if (divisible_by_power5(mu, -q)) {
      du0 = true;
    }
  }
  auto extra = static_cast<long unsigned>(-e2);
  auto extra_mask = static_cast<uint64_t>((1lu<<extra) - 1);
  auto fracl = dl & extra_mask;
  dl = dl >> extra;
  auto fracc = dc & extra_mask;
  dc = dc >> extra;
  auto fracu = du & extra_mask;
  du = du >> extra;
  auto uok = !du0 || fracu > 0;
  if (du0 && fracu == 0) {
    uok = (mant&1) == 0;
  }
  if (!uok) {
    --du;
  }
  auto cup = false;
  if (dc0) {
    cup = fracc > 1lu<<(extra-1) || (fracc == 1lu<<(extra-1) && dc&1 == 0);
  } else {
    cup = fracc>>(extra-1) == 1;
  }
  auto lok = dl0 && fracl == 0 && (mant&1) == 0;
  if (!lok) {
    ++dl;
  }
  auto c0 = dc0 && fracc == 0;
  ryu_digits(d, dl, dc, du, c0, cup);
  d.dp -= q;
  return d;
}

}  // namespace bongo::strconv::detail
