// Copyright The Go Authors.

#include <algorithm>
#include <bit>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>

#include "bongo/bongo.h"
#include "bongo/bytes.h"
#include "bongo/crypto/sha1/detail.h"
#include "bongo/crypto/sha1/error.h"
#include "bongo/crypto/sha1/sha1.h"
#include "bongo/encoding/binary.h"

namespace binary = bongo::encoding::binary;
using namespace std::string_view_literals;

namespace bongo::crypto::sha1 {
namespace {

std::string_view const magic = "sha\x01"sv;
size_t marshaled_size = magic.size() + 5*sizeof (uint32_t) + detail::chunk + sizeof (uint64_t);

std::pair<std::span<uint8_t const>, uint32_t> consume_uint32(std::span<uint8_t const> b) {
  auto x = binary::big_endian.uint32(b);
  return {b.subspan(4), x};
}

std::pair<std::span<uint8_t const>, uint64_t> consume_uint64(std::span<uint8_t const> b) {
  auto x = binary::big_endian.uint64(b);
  return {b.subspan(8), x};
}

}  // namespace

size_t hash::size() const noexcept {
  return sha1::size;
}

size_t hash::block_size() const noexcept {
  return sha1::block_size;
}

void hash::reset() noexcept {
  h_[0] = detail::init0;
  h_[1] = detail::init1;
  h_[2] = detail::init2;
  h_[3] = detail::init3;
  h_[4] = detail::init4;
  nx_ = 0;
  len_ = 0;
}

std::pair<long, std::error_code> hash::write(std::span<uint8_t const> p) {
  auto nn = p.size();
  len_ += static_cast<uint64_t>(nn);
  if (nx_ > 0) {
    auto x = std::span{x_}.subspan(nx_);
    auto n = std::min(x.size(), p.size());
    std::copy_n(p.begin(), n, x.begin());
    nx_ += n;
    if (nx_ == detail::chunk) {
      block(x_);
      nx_ = 0;
    }
    p = p.subspan(n);
  }
  if (p.size() >= detail::chunk) {
    auto n = p.size() & ~(detail::chunk - 1);
    block(p.subspan(0, n));
    p = p.subspan(n);
  }
  if (p.size() > 0) {
    auto x = std::span{x_};
    nx_ = p.size();
    std::copy(p.begin(), p.end(), x.begin());
  }
  return {nn, nil};
}

std::vector<uint8_t> hash::sum() {
  auto digest = std::vector<uint8_t>(sha1::size);
  auto h = *this;
  h.check_sum(digest);
  return digest;
}

std::span<uint8_t> hash::sum(std::span<uint8_t> digest) {
  if (digest.size() < sha1::size) {
    throw std::invalid_argument{"input buffer is too small"};
  }
  auto h = *this;
  return h.check_sum(digest);
}

std::vector<uint8_t> hash::constant_time_sum() {
  auto digest = std::vector<uint8_t>(sha1::size);
  auto h = *this;
  h.const_sum(digest);
  return digest;
}

std::span<uint8_t> hash::constant_time_sum(std::span<uint8_t> digest) {
  if (digest.size() < sha1::size) {
    throw std::invalid_argument{"input buffer is too small"};
  }
  auto h = *this;
  return h.const_sum(digest);
}

std::pair<std::vector<uint8_t>, std::error_code> hash::marshal_binary() const {
  auto b = std::vector<uint8_t>{};
  b.reserve(marshaled_size);
  auto out = std::back_inserter(b);
  std::copy(magic.begin(), magic.end(), out);
  binary::big_endian.put(h_[0], out);
  binary::big_endian.put(h_[1], out);
  binary::big_endian.put(h_[2], out);
  binary::big_endian.put(h_[3], out);
  binary::big_endian.put(h_[4], out);
  auto x = std::span{x_}.subspan(0, nx_);
  out = std::copy(x.begin(), x.end(), out);
  x = std::span{x_};
  std::fill_n(out, x.size()-nx_, 0);
  binary::big_endian.put(len_, out);
  return {b, nil};
}

std::error_code hash::unmarshal_binary(std::span<uint8_t const> b) {
  if (b.size() < magic.size() || bytes::to_string(b, magic.size()) != magic) {
    return error::invalid_hash_state_identifier;
  }
  if (b.size() < marshaled_size) {
    return error::invalid_hash_state_size;
  }
  b = b.subspan(magic.size());
  std::tie(b, h_[0]) = consume_uint32(b);
  std::tie(b, h_[1]) = consume_uint32(b);
  std::tie(b, h_[2]) = consume_uint32(b);
  std::tie(b, h_[3]) = consume_uint32(b);
  std::tie(b, h_[4]) = consume_uint32(b);
  auto x = std::span{x_};
  std::copy_n(b.begin(), x.size(), x.begin());
  b = b.subspan(x.size());
  std::tie(b, len_) = consume_uint64(b);
  nx_ = static_cast<long>(len_ % detail::chunk);
  return nil;
}

std::span<uint8_t> hash::check_sum(std::span<uint8_t> digest) {
  uint64_t len = len_;

  // Padding.
  uint8_t tmp[64] = {0};
  tmp[0] = 0x80;
  if (len%64 < 56) {
    write(std::span{tmp}.subspan(0, 56-len%64));
  } else {
    write(std::span{tmp}.subspan(0, 64+56-len%64));
  }

  // Length in bits.
  len <<= 3;
  binary::big_endian.put(tmp, len);
  write(std::span{tmp}.subspan(0, 8));

  if (nx_ != 0) {
    throw std::logic_error{"internal error: nx_ != 0"};
  }

  binary::big_endian.put(digest.subspan(0), h_[0]);
  binary::big_endian.put(digest.subspan(4), h_[1]);
  binary::big_endian.put(digest.subspan(8), h_[2]);
  binary::big_endian.put(digest.subspan(12), h_[3]);
  binary::big_endian.put(digest.subspan(16), h_[4]);

  return digest.subspan(0, sha1::size);
}

std::span<uint8_t> hash::const_sum(std::span<uint8_t> digest) {
  uint8_t length[8];
  auto l = len_ << 3;
  for (long unsigned i = 0; i < 8; ++i) {
    length[i] = static_cast<uint8_t>(l >> (56 - 8*i));
  }

  auto nx = static_cast<uint8_t>(nx_);
  auto t = nx - 56;
  auto mask1b = static_cast<uint8_t>(static_cast<int8_t>(t) >> 7);

  uint8_t separator = 0x80;
  for (uint8_t i = 0; i < detail::chunk; ++i) {
    auto mask = static_cast<uint8_t>(static_cast<int8_t>(i-nx) >> 7);

    x_[i] = (~mask & separator) | (mask & x_[i]);

    separator &= mask;

    if (i >= 56) {
      x_[i] |= mask1b & length[i-56];
    }
  }

  block(x_);
  auto h = std::span{h_};

  for (size_t i = 0; i < h.size(); ++i) {
    digest[i*4] = mask1b & static_cast<uint8_t>(h[i]>>24);
    digest[i*4+1] = mask1b & static_cast<uint8_t>(h[i]>>16);
    digest[i*4+2] = mask1b & static_cast<uint8_t>(h[i]>>8);
    digest[i*4+3] = mask1b & static_cast<uint8_t>(h[i]);
  }

  for (uint8_t i = 0; i < detail::chunk; ++i) {
    if (i < 56) {
      x_[i] = separator;
      separator = 0;
    } else {
      x_[i] = length[i-56];
    }
  }

  block(x_);

  for (size_t i = 0; i < h.size(); ++i) {
    digest[i*4] |= ~mask1b & static_cast<uint8_t>(h[i]>>24);
    digest[i*4+1] |= ~mask1b & static_cast<uint8_t>(h[i]>>16);
    digest[i*4+2] |= ~mask1b & static_cast<uint8_t>(h[i]>>8);
    digest[i*4+3] |= ~mask1b & static_cast<uint8_t>(h[i]);
  }

  return digest.subspan(0, sha1::size);
}

void hash::block(std::span<uint8_t const> p) {
  static const uint32_t K0 = 0x5a827999;
  static const uint32_t K1 = 0x6ed9eba1;
  static const uint32_t K2 = 0x8f1bbcdc;
  static const uint32_t K3 = 0xca62c1d6;

  uint32_t w[16] = {0};
  auto h0 = h_[0];
  auto h1 = h_[1];
  auto h2 = h_[2];
  auto h3 = h_[3];
  auto h4 = h_[4];

  while (p.size() >= detail::chunk) {
    for (auto i = 0; i < 16; ++i) {
      auto j = i * 4;
      w[i] = (static_cast<uint32_t>(p[j])<<24) | (static_cast<uint32_t>(p[j+1])<<16) |
	(static_cast<uint32_t>(p[j+2])<<8) | static_cast<uint32_t>(p[j+3]);
    }

    auto a = h0;
    auto b = h1;
    auto c = h2;
    auto d = h3;
    auto e = h4;

    auto i = 0;
    for (; i < 16; ++i) {
      auto f = (b&c) | ((~b)&d);
      auto t = std::rotl(a, 5) + f + e + w[i&0xf] + K0;
      e = d;
      d = c;
      c = std::rotl(b, 30);
      b = a;
      a = t;
    }
    for (; i < 20; ++i) {
      auto tmp = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
      w[i&0xf] = (tmp<<1) | (tmp>>(32-1));
      auto f = (b&c) | ((~b)&d);
      auto t = std::rotl(a, 5) + f + e + w[i&0xf] + K0;
      e = d;
      d = c;
      c = std::rotl(b, 30);
      b = a;
      a = t;
    }
    for (; i < 40; ++i) {
      auto tmp = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
      w[i&0xf] = (tmp<<1) | (tmp>>(32-1));
      auto f = b ^ c ^ d;
      auto t = std::rotl(a, 5) + f + e + w[i&0xf] + K1;
      e = d;
      d = c;
      c = std::rotl(b, 30);
      b = a;
      a = t;
    }
    for (; i < 60; ++i) {
      auto tmp = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
      w[i&0xf] = (tmp<<1) | (tmp>>(32-1));
      auto f = ((b | c) & d) | (b & c);
      auto t = std::rotl(a, 5) + f + e + w[i&0xf] + K2;
      e = d;
      d = c;
      c = std::rotl(b, 30);
      b = a;
      a = t;
    }
    for (; i < 80; ++i) {
      auto tmp = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[(i)&0xf];
      w[i&0xf] = (tmp<<1) | (tmp>>(32-1));
      auto f = b ^ c ^ d;
      auto t = std::rotl(a, 5) + f + e + w[i&0xf] + K3;
      e = d;
      d = c;
      c = std::rotl(b, 30);
      b = a;
      a = t;
    }

    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
    h4 += e;

    p = p.subspan(detail::chunk);
  }

  h_[0] = h0;
  h_[1] = h1;
  h_[2] = h2;
  h_[3] = h3;
  h_[4] = h4;
}

std::vector<uint8_t> sum(std::span<uint8_t const> data) {
  auto d = hash{};
  d.write(data);
  return d.sum();
}

}  // namespace bongo::crypto
