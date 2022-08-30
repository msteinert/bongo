// Copyright The Go Authors.

#pragma once

#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/bytes/bytes.h>
#include <bongo/io/error.h>

namespace bongo::io {

constexpr static long seek_start = 0;
constexpr static long seek_current = 1;
constexpr static long seek_end = 2;

// Reader is the interface that wraps the basic read method.
template <typename T>
concept Reader = requires (T r, std::span<uint8_t> p) {
  { r.read(p) } -> std::same_as<std::pair<long, std::error_code>>;
};

template <typename T> requires Reader<T>
std::pair<long, std::error_code> read(T& r, std::span<uint8_t> p) {
  return r.read(p);
}

template <typename T>
concept ReaderFunc = requires (T r, std::span<uint8_t> p) {
  { read(r, p) } -> std::same_as<std::pair<long, std::error_code>>;
};

// Writer is the interface that wraps the basic write method.
template <typename T>
concept Writer = requires (T w, std::span<uint8_t const> const p) {
  { w.write(p) } -> std::same_as<std::pair<long, std::error_code>>;
};

template <Writer T>
std::pair<long, std::error_code> write(T& w, std::span<uint8_t const> const p) {
  return w.write(p);
}

template <typename T>
concept WriterFunc = requires (T w, std::span<uint8_t const> const p) {
  { write(w, p) } -> std::same_as<std::pair<long, std::error_code>>;
};

// Closer is the interface that wraps the basic close method.
template <typename T>
concept Closer = requires (T c) {
  { c.close() };
};

template <typename T> requires Closer<T>
std::error_code close(T& c) {
  return c.close();
}

template <typename T>
concept CloserFunc = requires (T c) {
  { close(c) };
};

// Seeker is the interface that wraps the basic seek method.
template <typename T>
concept Seeker = requires (T s, int64_t offset, long whence) {
  { s.seek(offset, whence) } -> std::same_as<std::pair<int64_t, std::error_code>>;
};

template <typename T> requires Seeker<T>
std::pair<int64_t, std::error_code> seek(T& s, int64_t offset, long whence) {
  return s.seek(offset, whence);
}

template <typename T>
concept SeekerFunc = requires (T s, int64_t offset, long whence) {
  { seek(s, offset, whence) } -> std::same_as<std::pair<int64_t, std::error_code>>;
};

// ReadWriter is the interface that groups the basic read and write methods.
template <typename T>
concept ReadWriter = requires {
  requires Reader<T> && Writer<T>;
};

template <typename T>
concept ReadWriterFunc = requires {
  requires ReaderFunc<T> && WriterFunc<T>;
};

// ReadCloser is the interface that groups the basic read and close methods.
template <typename T>
concept ReadCloser = requires {
  requires Reader<T> && Closer<T>;
};

template <typename T>
concept ReadCloserFunc = requires {
  requires ReaderFunc<T> && CloserFunc<T>;
};

// WriteCloser is the interface that groups the basic write and close methods.
template <typename T>
concept WriteCloser = requires {
  requires Writer<T> && Closer<T>;
};

template <typename T>
concept WriteCloserFunc = requires {
  requires WriterFunc<T> && CloserFunc<T>;
};

// ReadWriteCloser is the interface the groups the basic read, write, and close methods.
template <typename T>
concept ReadWriteCloser = requires {
  requires Reader<T> && Writer<T> && Closer<T>;
};

template <typename T>
concept ReadWriteCloserFunc = requires {
  requires ReaderFunc<T> && WriterFunc<T> && CloserFunc<T>;
};

// ReadSeeker is the interface that groups the basic read and seek methods.
template <typename T>
concept ReadSeeker = requires {
  requires Reader<T> && Seeker<T>;
};

template <typename T>
concept ReadSeekerFunc = requires {
  requires ReaderFunc<T> && SeekerFunc<T>;
};

// WriteSeeker is the interface that groups the basic write and seek methods.
template <typename T>
concept WriteSeeker = requires {
  requires Writer<T> && Seeker<T>;
};

template <typename T>
concept WriteSeekerFunc = requires {
  requires WriterFunc<T> && SeekerFunc<T>;
};

// ReadWriteSeeker is the interface that groups the basic read, write, and seek methods.
template <typename T>
concept ReadWriteSeeker = requires {
  requires Reader<T> && Writer<T> && Seeker<T>;
};

template <typename T>
concept ReadWriteSeekerFunc = requires {
  requires ReaderFunc<T> && WriterFunc<T> && SeekerFunc<T>;
};

// ReaderFrom is the interface that wraps the read_from method.
template <typename T, typename U>
concept ReaderFrom = requires (T w, U r) {
  { w.read_from(r) } -> std::same_as<std::pair<int64_t, std::error_code>>;
};

template <typename T, typename U> requires ReaderFrom<T, U>
std::pair<int64_t, std::error_code> read_from(T& rf, U& r) {
  return rf.read_from(r);
}

template <typename T, typename U>
concept ReaderFromFunc = requires (T w, U r) {
  { read_from(w, r) } -> std::same_as<std::pair<int64_t, std::error_code>>;
};

// WriterTo is the interface that wraps the write_to method.
template <typename T, typename U>
concept WriterTo = requires (T r, U w) {
  { r.write_to(w) } -> std::same_as<std::pair<int64_t, std::error_code>>;
};

template <typename T, typename U> requires WriterTo<T, U>
std::pair<int64_t, std::error_code> write_to(T& r, U& w) {
  return r.write_to(w);
}

template <typename T, typename U>
concept WriterToFunc = requires (T r, U w) {
  { write_to(r, w) } -> std::same_as<std::pair<int64_t, std::error_code>>;
};

// ReaderAt is the interface that wraps the basic read_at method.
template <typename T>
concept ReaderAt = requires (T r, std::span<uint8_t> p, int64_t off) {
  { r.read_at(p, off) } -> std::same_as<std::pair<long, std::error_code>>;
};

template <typename T> requires ReaderAt<T>
std::pair<long, std::error_code> read_at(T& r, std::span<uint8_t> p, int64_t off) {
  return r.read_at(p, off);
}

template <typename T>
concept ReaderAtFunc = requires (T r, std::span<uint8_t> p, int64_t off) {
  { read_at(r, p, off) } -> std::same_as<std::pair<long, std::error_code>>;
};

// WriterAt is the interface that wraps the basic write_at method.
template <typename T>
concept WriterAt = requires (T w, std::span<uint8_t const> p, int64_t off) {
  { w.write_at(p, off) } -> std::same_as<std::pair<long, std::error_code>>;
};

template <typename T> requires WriterAt<T>
std::pair<long, std::error_code> write_at(T& w, std::span<uint8_t const> p, int64_t off) {
  return w.write_at(p, off);
}

template <typename T>
concept WriterAtFunc = requires (T w, std::span<uint8_t const> p, int64_t off) {
  { write_at(w, p, off) } -> std::same_as<std::pair<long, std::error_code>>;
};

// ByteReader is the interface that wraps the read_byte method.
template <typename T>
concept ByteReader = requires (T r) {
  { r.read_byte() } -> std::same_as<std::pair<uint8_t, std::error_code>>;
};

template <typename T> requires ByteReader<T>
std::pair<uint8_t, std::error_code> read_byte(T& r) {
  return r.read_byte();
}

template <typename T>
concept ByteReaderFunc = requires (T r) {
  { read_byte(r) } -> std::same_as<std::pair<uint8_t, std::error_code>>;
};

// ByteUnreader is the interface that wraps the unread_byte method.
template <typename T>
concept ByteUnreader = requires (T r) {
  { r.unread_byte() } -> std::same_as<std::error_code>;
};

template <typename T> requires ByteUnreader<T>
std::error_code unread_byte(T& r) {
  return r.unread_byte();
}

template <typename T>
concept ByteUnreaderFunc = requires (T r) {
  { unread_byte(r) } -> std::same_as<std::error_code>;
};

// ByteWriter is the interface that wraps the write_byte method.
template <typename T>
concept ByteWriter = requires (T w, uint8_t c) {
  { write_byte(w, c) } -> std::same_as<std::error_code>;
};

template <typename T> requires ByteWriter<T>
std::error_code write_byte(T& w, uint8_t c) {
  return w.write_byte(c);
}

template <typename T>
concept ByteWriterFunc = requires (T w, uint8_t c) {
  { write_byte(w, c) } -> std::same_as<std::error_code>;
};

// RuneReader is the interface that wraps the read_rune method.
template <typename T>
concept RuneReader = requires (T r) {
  { r.read_rune() } -> std::same_as<std::tuple<rune, long, std::error_code>>;
};

template <typename T> requires RuneReader<T>
std::tuple<rune, long, std::error_code> read_rune(T& r) {
  return r.read_rune();
}

template <typename T>
concept RuneReaderFunc = requires (T r) {
  { read_rune(r) } -> std::same_as<std::tuple<rune, long, std::error_code>>;
};

// RuneUnreader is the interface that wraps the unread_rune method.
template <typename T>
concept RuneUnreader = requires (T r) {
  { r.unread_rune() } -> std::same_as<std::error_code>;
};

template <typename T> requires RuneUnreader<T>
std::error_code unread_rune(T& r) {
  return r.unread_rune();
}

template <typename T>
concept RuneUnreaderFunc = requires (T r) {
  { unread_rune(r) } -> std::same_as<std::error_code>;
};

// StringWriter is the interface that wraps the write_string method.
template <typename T>
concept StringWriter = requires (T w, std::string_view s) {
  { w.write_string(s) } -> std::same_as<std::pair<long, std::error_code>>;
};

template <typename T> requires StringWriter<T> || WriterFunc<T>
std::pair<long, std::error_code> write_string(T& w, std::string_view s) {
  if constexpr (StringWriter<T>) {
    return w.write_string(s);
  }
  return write(w, bytes::to_bytes(s));
}

template <typename T>
concept StringWriterFunc = requires (T w, std::string_view s) {
  { write_string(w, s) } -> std::same_as<std::pair<long, std::error_code>>;
};

template <typename T> requires ReaderFunc<T>
std::pair<long, std::error_code> read_at_least(T& r, std::span<uint8_t> buf, long min) {
  if (buf.size() < static_cast<size_t>(min)) {
    return {0, error::short_buffer};
  }
  long n = 0;
  std::error_code err = nil;
  while (n < min && err == nil) {
    long nn;
    std::tie(nn, err) = read(r, buf.subspan(n));
    n += nn;
  }
  if (n >= min) {
    err = nil;
  } else if (n > 0 && err == eof) {
    err = error::unexpected_eof;
  }
  return {n, err};
}

template <typename T> requires ReaderFunc<T>
std::pair<long, std::error_code> read_at_least(T& r, std::span<uint8_t> buf) {
  return read_at_least(r, buf, static_cast<long>(buf.size()));
}

template <typename T> requires ReaderFunc<T>
std::pair<long, std::error_code> read_full(T& r, std::span<uint8_t> buf) {
  return read_at_least(r, buf, buf.size());
}

template <typename T> requires ReaderFunc<T>
struct limited_reader {
  T& r;
  int64_t n;

  limited_reader(T& r_, int64_t n_)
      : r{r_}
      , n{n_} {}

  std::pair<long, std::error_code> read(std::span<uint8_t> p) {
    if (n <= 0) {
      return {0, eof};
    }
    if (static_cast<int64_t>(p.size()) > n) {
      p = p.subspan(0, n);
    }
    using io::read;
    auto [nn, err] = read(r, p);
    n -= static_cast<int64_t>(nn);
    return {nn, err};
  }
};

template <typename T, typename U> requires WriterToFunc<U, T> || ReaderFromFunc<T, U> || (WriterFunc<T> && ReaderFunc<U>)
std::pair<int64_t, std::error_code> copy_buffer(T& dst, U& src, std::span<uint8_t> buf) {
  if constexpr (WriterToFunc<U, T>) {
    return write_to(src, dst);
  }
  if constexpr (ReaderFromFunc<T, U>) {
    return read_from(dst, src);
  }
  std::vector<uint8_t> vec;
  if (buf.size() == 0) {
    vec.resize(32*1024);
    buf = vec;
  }
  int64_t written = 0;
  std::error_code err = nil;
  for (;;) {
    auto [nr, er] = read(src, buf);
    if (nr > 0) {
      auto [nw, ew] = write(dst, buf.subspan(0, nr));
      if (nw < 0 || nr < nw) {
        nw = 0;
        if (ew == nil) {
          ew = error::invalid_write;
        }
      }
      written += static_cast<int64_t>(nw);
      if (ew != nil) {
        err = ew;
        break;
      }
      if (nr != nw) {
        err = error::short_write;
        break;
      }
    }
    if (er != nil) {
      if (er != eof) {
        err = er;
      }
      break;
    }
  }
  return {written, err};
}

template <typename T, typename U> requires WriterFunc<T> && ReaderFunc<U>
std::pair<int64_t, std::error_code> copy(T& dst, U& src) {
  return copy_buffer(dst, src, nil);
}

template <typename T, typename U> requires WriterFunc<T> && ReaderFunc<U>
std::pair<int64_t, std::error_code> copy_n(T& dst, U& src, int64_t n) {
  auto l = limited_reader{src, n};
  auto [written, err] = copy(dst, l);
  if (written == n) {
    return {n, nil};
  }
  if (written < n && !err) {
    err = eof;
  }
  return {written, err};
}

template <typename T> requires ReaderAtFunc<T>
class section_reader {
  T& r_;
  int64_t base_, off_, limit_;

 public:
  section_reader(T& r, int64_t off, int64_t n)
      : r_{r}
      , base_{off}
      , off_{off} {
    limit_ = off <= std::numeric_limits<int64_t>::max()-n
      ? n + off
      : std::numeric_limits<int64_t>::max();
  }

  size_t size() const noexcept {
    return limit_ - base_;
  }

  std::pair<long, std::error_code> read(std::span<uint8_t> p) {
    if (off_ >= limit_) {
      return {0, eof};
    }
    if (auto max = limit_ - off_; static_cast<int64_t>(p.size()) > max) {
      p = p.subspan(0, max);
    }
    auto [n, err] = io::read_at(r_, p, off_);
    off_ += static_cast<int64_t>(n);
    return {n, err};
  }

  std::pair<int64_t, std::error_code> seek(int64_t offset, long whence) {
    switch (whence) {
    default:
      return {0, error::whence};
    case seek_start:
      offset += base_;
      break;
    case seek_current:
      offset += off_;
      break;
    case seek_end:
      offset += limit_;
      break;
    }
    if (offset < base_) {
      return {0, error::offset};
    }
    off_ = offset;
    return {offset - base_, nil};
  }

  std::pair<long, std::error_code> read_at(std::span<uint8_t> p, int64_t off) {
    if (off < 0 || off >= limit_-base_) {
      return {0, eof};
    }
    off += base_;
    if (auto max = limit_ - off; static_cast<int64_t>(p.size()) > max) {
      p = p.subspan(0, max);
      auto [n, err] = io::read_at(r_, p, off);
      if (err == nil) {
        err = eof;
      }
      return {n, err};
    }
    return io::read_at(r_, p, off);
  }
};

template <typename T, typename U> requires ReaderFunc<T> && WriterFunc<U>
struct tee_reader {
  T& r;
  U& w;

  tee_reader(T& r_, U& w_)
      : r{r_}
      , w{w_} {}

  std::pair<long, std::error_code> read(std::span<uint8_t> p) {
    using io::read;
    auto [n, err] = read(r, p);
    if (n > 0) {
      if (std::tie(n, err) = write(w, p.subspan(0, n)); err != nil) {
        return {n, err};
      }
    }
    return {n, err};
  }
};

struct discard {
  std::pair<long, std::error_code> write(std::span<uint8_t const> p) {
    return {p.size(), nil};
  }
  std::pair<long, std::error_code> write_string(std::string_view s) {
    return {s.size(), nil};
  }
  template <typename T> requires io::Reader<T>
  auto read_from(T& r) -> std::pair<int64_t, std::error_code> {
    uint8_t buf[8192];
    int64_t n = 0;
    for (;;) {
      auto [read_size, err] = r.read(buf);
      n += read_size;
      if (err != nil) {
        if (err == eof) {
          return {n, nil};
        }
        return {n, err};
      }
    }
  }
};

template <typename T> requires ReaderFunc<T>
struct nop_closer {
  T& r;

  nop_closer(T& r_)
      : r{r_} {}

  std::error_code close() { return nil; }
};

template <typename T> requires ReaderFunc<T>
std::pair<std::vector<uint8_t>, std::error_code> read_all(T& r) {
  auto b = std::vector<uint8_t>(512, 0);
  long n = 0;
  for (;;) {
    if (static_cast<size_t>(n) == b.capacity()) {
      b.resize(b.size() + 512);
    }
    //auto [m, err] = read(r, std::span{std::next(b.begin(), n), b.end()});
    auto [m, err] = read(r, std::span{std::next(b.begin(), n), b.begin()+b.size()});
    n += m;
    if (err != nil) {
      b.resize(n);
      if (err == io::eof) {
        err = nil;
      }
      return {b, err};
    }
  }
}

}  // namespace bongo::io
