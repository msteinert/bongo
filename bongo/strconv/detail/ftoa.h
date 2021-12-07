// Copyright The Go Authors.

#pragma once

#include <algorithm>
#include <iterator>
#include <type_traits>

#include <bongo/math/bits.h>
#include <bongo/strconv/detail/decimal.h>
#include <bongo/strconv/detail/float_info.h>
#include <bongo/strconv/detail/itoa.h>
#include <bongo/strconv/quote.h>

namespace bongo::strconv::detail {

// %e: -d.ddddde±dd
template <std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt fmt_e(OutputIt out, bool neg, decimal const& d, long prec, char fmt) {
  // sign
  if (neg) {
    *out++ = '-';
  }

  // first digit
  auto ch = '0';
  if (d.nd != 0) {
    ch = d.d[0];
  }
  *out++ = ch;

  // .moredigits
  if (prec > 0) {
    *out++ = '.';
    auto i = 1;
    auto m = std::min(d.nd, prec+1);
    if (i < m) {
      out = std::copy(std::next(d.d.begin(), i), std::next(d.d.begin(), m), out);
      i = m;
    }
    for (; i <= prec; ++i) {
      *out++ = '0';
    }
  }

  // e±
  *out++ = fmt;
  auto exp = d.dp - 1;
  if (d.nd == 0) {
    exp = 0;
  }
  if (exp < 0) {
    ch = '-';
    exp = -exp;
  } else {
    ch = '+';
  }
  *out++ = ch;

  // dd or ddd
  if (exp < 10) {
    *out++ = '0';
    *out++ = static_cast<char>(exp)+'0';
  } else if (exp < 100) {
    *out++ = static_cast<char>(exp/10)+'0';
    *out++ = static_cast<char>(exp%10)+'0';
  } else {
    *out++ = static_cast<char>(exp/100)+'0';
    *out++ = static_cast<char>(exp/10)%10+'0';
    *out++ = static_cast<char>(exp%10)+'0';
  }

  return out;
}

// %f: -ddddddd.ddddd
template <std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt fmt_f(OutputIt out, bool neg, decimal const& d, long prec) {
  // sign
  if (neg) {
    *out++ = '-';
  }

  // integer, padded with zeros as needed.
  if (d.dp > 0) {
    auto m = std::min(d.nd, d.dp);
    out = std::copy(d.d.begin(), std::next(d.d.begin(), m), out);
    for (; m < d.dp; ++m) {
      *out++ = '0';
    }
  } else {
    *out++ = '0';
  }

  // fraction
  if (prec > 0) {
    *out++ = '.';
    for (auto i = 0; i < prec; ++i) {
      auto ch = '0';
      if (auto j = d.dp + i; 0 <= j && j < d.nd) {
        ch = d.d[j];
      }
      *out++ = ch;
    }
  }

  return out;
}

// %b: -ddddddddp±ddd
template <typename T, std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt fmt_b(OutputIt out, bool neg, uint64_t mant, long exp) {
  // sign
  if (neg) {
    *out++ = '-';
  }

  // mantissa
  out = format_bits(mant, out, 10);

  // p
  *out++ = 'p';

  // ±exponent
  exp -= static_cast<long>(float_info<T>::mantissa_bits());
  if (exp >= 0) {
    *out++ = '+';
  }
  return format_bits(exp, out, 10);
}

// %x: -0x1.yyyyyyyyp±ddd or -0x0p+0. (y is hex digit, d is decimal digit)
template <typename T, std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt fmt_x(OutputIt out, long prec, char fmt, bool neg, uint64_t mant, long exp) {
  if (mant == 0) {
    exp = 0;
  }

  // Shift digits so leading (if any) is at bit 1<<60;
  mant <<= 60 - float_info<T>::mantissa_bits();
  while (mant != 0 && mant&(1llu<<60) == 0) {
    mant <<= 1;
    --exp;
  }

  // Round
  if (prec >= 0 && prec < 15) {
    auto shift = static_cast<uint64_t>(prec * 4);
    auto extra = static_cast<uint64_t>(mant << shift) & ((1llu<<60) - 1);
    mant >>= (60 - shift);
    if ((extra|(mant&1)) > (1llu<<59)) {
      ++mant;
    }
    mant <<= (60 - shift);
    if ((mant&(1llu<<61)) != 0) {
      mant >>= 1;
      ++exp;
    }
  }

  // Select character set
  auto hex = fmt == 'x'
    ? detail::lower_hex
    : detail::upper_hex;

  // Sign, 0x, leading digit
  if (neg) {
    *out++ = '-';
  }
  *out++ = '0';
  *out++ = fmt;
  *out++ = '0'+static_cast<char>((mant>>60)&1);

  // .fraction
  mant <<= 4;
  if (prec < 0 && mant != 0) {
    *out++ = '.';
    while (mant != 0) {
      *out++ = hex[(mant>>60)&15];
      mant <<= 4;
    }
  } else if (prec > 0) {
    *out++ = '.';
    for (long i = 0; i < prec; ++i) {
      *out++ = hex[(mant>>60)&15];
      mant <<= 4;
    }
  }

  // p±
  *out++ = fmt == 'x' ? 'p' : 'P';
  *out++ = exp < 0 ? '-' : '+';

  // dd or ddd or dddd
  if (exp < 100) {
    *out++ = static_cast<char>(exp/10) + '0';
    *out++ = static_cast<char>(exp%10) + '0';
  } else if (exp < 1000) {
    *out++ = static_cast<char>(exp/100) + '0';
    *out++ = static_cast<char>((exp%10)%10) + '0';
    *out++ = static_cast<char>(exp%10) + '0';
  } else {
    *out++ = static_cast<char>(exp/1000) + '0';
    *out++ = static_cast<char>((exp%100)%10) + '0';
    *out++ = static_cast<char>((exp%10)%10) + '0';
    *out++ = static_cast<char>(exp%10) + '0';
  }

  return out;
}

template <typename T>
constexpr void round_shortest(decimal& d, uint64_t mant, long exp) {
  if (mant == 0) {
    d.nd = 0;
    return;
  }
  auto minexp = float_info<T>::bias() + 1;
  if (exp > minexp && 332*(d.dp-d.nd) >= 100*(exp-static_cast<long>(float_info<T>::mantissa_bits()))) {
    return;
  }
  auto upper = decimal{mant*2 + 1};
  upper.shift(exp - static_cast<long>(float_info<T>::mantissa_bits()) - 1);
  uint64_t mantlo = 0;
  long explo = 0;
  if (mant > (1llu<<float_info<T>::mantissa_bits()) || exp == minexp) {
    mantlo = mant - 1;
    explo = exp;
  } else {
    mantlo = mant*2 - 1;
    explo = exp;
  }
  auto lower = decimal{mantlo*2 + 1};
  lower.shift(explo - static_cast<long>(float_info<T>::mantissa_bits()) - 1);
  auto inclusive = mant%2 == 0;
  uint8_t upperdelta = 0;
  for (long ui = 0;; ++ui) {
    auto mi = ui - upper.dp + d.dp;
    if (mi >= d.nd) {
      break;
    }
    auto li = ui - upper.dp + lower.dp;
    auto l = '0';  // lower digit
    if (li >= 0 && li < lower.nd) {
      l = lower.d[li];
    }
    auto m = '0';  // middle digit
    if (mi >= 0) {
      m = d.d[mi];
    }
    auto u = '0';  // upper digit
    if (ui < upper.nd) {
      u = upper.d[ui];
    }
    auto okdown = l != m || inclusive && li+1 == lower.nd;
    if (upperdelta == 0 && m+1 < u) {
      upperdelta = 2;
    } else if (upperdelta == 0 && m != u) {
      upperdelta = 1;
    } else if (upperdelta == 1 && (m != '9' || u != '0')) {
      upperdelta = 2;
    }
    auto okup = upperdelta > 0 && (inclusive || upperdelta > 1 || ui+1 < upper.nd);
    if (okdown && okup) {
      d.round(mi + 1);
      return;
    }  else if (okdown) {
      d.round_down(mi +1);
      return;
    } else if (okup) {
      d.round_up(mi + 1);
      return;
    }
  }
}

template <std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt format_decimal(OutputIt out, bool shortest, bool neg, decimal const& d, long prec, char fmt) {
  switch (fmt) {
  case 'e':
  case 'E':
    return fmt_e(out, neg, d, prec, fmt);
  case 'f':
    return fmt_f(out, neg, d, prec);
  case 'g':
  case 'G':
    auto eprec = prec;
    if (eprec > d.nd && d.nd >= d.dp) {
      eprec = d.nd;
    }
    if (shortest) {
      eprec = 6;
    }
    auto exp = d.dp - 1;
    if (exp < -4 || exp >= eprec) {
      if (prec > d.nd) {
        prec = d.nd;
      }
      return fmt_e(out, neg, d, prec-1, fmt+'e'-'g');
    }
    if (prec > d.dp) {
      prec = d.nd;
    }
    return fmt_f(out, neg, d, std::max(prec-d.dp, static_cast<long>(0)));
  }
  // unknown format
  *out++ = '%';
  *out++ = fmt;
  return out;
}

template <typename T, std::output_iterator<uint8_t> OutputIt>
constexpr OutputIt big_ftoa(OutputIt out, long prec, char fmt, bool neg, uint64_t mant, long exp) {
  auto d = detail::decimal{mant};
  d.shift(exp - static_cast<long>(float_info<T>::mantissa_bits()));
  auto shortest = prec < 0;
  if (shortest) {
    round_shortest<T>(d, mant, exp);
    switch (fmt) {
    case 'e':
    case 'E':
      prec = d.nd - 1;
      break;
    case 'f':
      prec = std::max(d.nd-d.dp, 0l);
      break;
    case 'g':
    case 'G':
      prec = d.nd;
      break;
    }
  } else {
    switch (fmt) {
    case 'e':
    case 'E':
      d.round(prec + 1);
      break;
    case 'f':
      d.round(d.dp + prec);
      break;
    case 'g':
    case 'G':
      if (prec == 0) {
        prec = 1;
      }
      d.round(prec);
      break;
    }
  }
  return format_decimal(out, shortest, neg, d, prec, fmt);
}

}  // namespace bongo::strconv::detail
