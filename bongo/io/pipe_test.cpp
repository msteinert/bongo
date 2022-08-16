// Copyright The Go Authors.

#include <chrono>
#include <span>
#include <thread>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/bongo.h"
#include "bongo/bytes.h"
#include "bongo/io.h"
#include "bongo/strings.h"

namespace bongo::io {

using namespace std::chrono_literals;
using namespace std::string_literals;

struct pipe_return {
  int n;
  std::error_code err;
};

template <Writer T>
void check_write(T& w, std::span<uint8_t const> data, chan<pipe_return>& c) {
  auto [n, err] = w.write(data);
  c << pipe_return{n, err};
}

TEST_CASE("Pipe: single read/write pair", "[io]") {
  auto c = chan<pipe_return>{};
  pipe_reader r;
  pipe_writer w;
  std::tie(r, w) = make_pipe();
  auto buf = std::vector<uint8_t>(64, 0);
  auto t = std::thread{[&]() {
    check_write(w, bytes::to_bytes(std::string_view{"hello, world"}), c);
  }};

  auto [n, err] = r.read(buf);
  CHECK(n == 12);
  CHECK(bytes::to_string(buf, n) == "hello, world");
  CHECK(err == nil);

  auto pr = pipe_return{};
  pr << c;
  CHECK(pr.n == 12);
  CHECK(pr.err == nil);
  r.close();
  w.close();
  t.join();
}

template <Reader T>
void reader(T& r, chan<pipe_return>& c) {
  auto buf = std::vector<uint8_t>(64);
  for (;;) {
    auto [n, err] = r.read(buf);
    if (err == eof) {
      c << pipe_return{n, err};
      break;
    }
    c << pipe_return{n, err};
  }
}

TEST_CASE("Pipe: sequence of read/write pairs", "[io]") {
  auto c = chan<pipe_return>{};
  pipe_reader r;
  pipe_writer w;
  std::tie(r, w) = make_pipe();
  auto t = std::thread{[&] {
    reader(r, c);
  }};

  auto buf = std::vector<uint8_t>(64);
  for (auto i = 0; i < 5; ++i) {
    auto p = std::span{buf}.subspan(0, 5+i*10);
    auto [n, err] = w.write(p);
    CHECK(static_cast<size_t>(n) == p.size());
    CHECK(err == nil);
    auto pr = pipe_return{};
    pr << c;
    CHECK(pr.n == n);
    CHECK(pr.err == nil);
  }

  w.close();
  auto pr = pipe_return{};
  pr << c;
  CHECK(pr.n == 0);
  CHECK(pr.err == io::eof);
  t.join();
}

template <typename T>
concept WriteCloser = requires {
  requires Writer<T> && Closer<T>;
};

template <WriteCloser T>
void writer(T& w, std::vector<uint8_t> buf, chan<pipe_return>& c) {
  auto [n, err] = w.write(buf);
  w.close();
  c << pipe_return{n, err};
}

TEST_CASE("Pipe: large write that requires multiple reads to satisfy", "[io]") {
  auto c = chan<pipe_return>{};
  pipe_reader r;
  pipe_writer w;
  std::tie(r, w) = make_pipe();
  auto wdat = std::vector<uint8_t>(128);
  for (size_t i = 0; i < wdat.size(); ++i) {
    wdat[i] = static_cast<uint8_t>(i);
  }

  auto t = std::thread{[&]() {
    writer(w, wdat, c);
  }};

  auto rdat = std::vector<uint8_t>(1024);
  size_t tot = 0;
  for (auto n = 1; n <= 256; n *= 2) {
    auto [nn, err] = r.read(std::span{rdat}.subspan(tot, n));
    CHECK((err == nil || err == eof));

    auto expect = n;
    if (n == 128) {
      expect = 1;
    } else if (n == 256) {
      expect = 0;
      CHECK(err == eof);
    }
    CHECK(nn == expect);
    tot += nn;
  }

  auto pr = pipe_return{};
  pr << c;
  CHECK(pr.n == 128);
  CHECK(pr.err == nil);
  CHECK(tot == 128);
  for (auto i = 0; i < 128; ++i) {
    CHECK(rdat[i] == static_cast<uint8_t>(i));
  }
  t.join();
}

struct pipe_test {
  bool async = false;
  std::error_code err = nil;
  bool close_with_error = false;
  std::string str() {
    return "async="s + (async ? "true" : "false") +
      " err=" + err.message() +
      " close_with_error=" + (close_with_error ? "true" : "false");
  }
};

template <Closer T>
void delay_close(T& cl, chan<std::error_code>& ch, pipe_test& tt) {
  std::this_thread::sleep_for(1ms);
  std::error_code err = nil;
  if (tt.close_with_error) {
    err = cl.close_with_error(tt.err);
  } else {
    err = cl.close();
  }
  ch << err;
}

TEST_CASE("Pipe: after close", "[io]") {
  auto test_cases = std::vector<pipe_test>{
    {true, nil, false},
    {true, nil, true},
    {true, error::short_write, true},
    {false, nil, false},
    {false, nil, true},
    {false, error::short_write, true},
  };
  SECTION("Read after close") {
    for (auto& tt : test_cases) {
      std::thread t;
      auto c = chan<std::error_code>{1};
      pipe_reader r;
      pipe_writer w;
      std::tie(r, w) = make_pipe();
      if (tt.async) {
        t = std::thread{[&]() {
          delay_close(w, c, tt);
        }};
      } else {
        delay_close(w, c, tt);
      }
      auto buf = std::vector<uint8_t>(64);
      auto [n, err] = r.read(buf);
      std::error_code err1;
      err1 << c;
      CHECK(err1 == nil);
      auto want = tt.err == nil ? eof : tt.err;
      CHECK(err == want);
      CHECK(n == 0);
      err = r.close();
      CHECK(err == nil);
      if (t.joinable()) {
        t.join();
      }
    }
  }
  SECTION("Write after close") {
    for (auto& tt : test_cases) {
      std::thread t;
      auto c = chan<std::error_code>{1};
      pipe_reader r;
      pipe_writer w;
      std::tie(r, w) = make_pipe();
      if (tt.async) {
        t = std::thread{[&]() {
          delay_close(r, c, tt);
        }};
      } else {
        delay_close(r, c, tt);
      }
      auto [n, err] = write_string(w, "hello, world");
      std::error_code err1;
      err1 << c;
      CHECK(err1 == nil);
      auto want = tt.err == nil ? error::closed_pipe : tt.err;
      CHECK(err == want);
      CHECK(n == 0);
      err = w.close();
      CHECK(err == nil);
      if (t.joinable()) {
        t.join();
      }
    }
  }
}

TEST_CASE("Pipe: close on read side during read", "[io]") {
  auto c = chan<std::error_code>{1};
  pipe_reader r;
  pipe_writer w;
  std::tie(r, w) = make_pipe();
  auto tt = pipe_test{};
  auto t = std::thread{[&]() {
    delay_close(r, c, tt);
  }};
  auto buf = std::vector<uint8_t>(64);
  auto [n, err] = r.read(buf);
  std::error_code err1;
  err1 << c;
  CHECK(err1 == nil);
  CHECK(n == 0);
  CHECK(err == error::closed_pipe);
  t.join();
}

TEST_CASE("Pipe: close on write side during write", "[io]") {
  auto c = chan<std::error_code>{1};
  pipe_reader r;
  pipe_writer w;
  std::tie(r, w) = make_pipe();
  auto tt = pipe_test{};
  auto t = std::thread{[&]() {
    delay_close(w, c, tt);
  }};
  auto buf = std::vector<uint8_t>(64);
  auto [n, err] = w.write(buf);
  std::error_code err1;
  err1 << c;
  CHECK(err1 == nil);
  CHECK(n == 0);
  CHECK(err == error::closed_pipe);
  t.join();
}

TEST_CASE("Pipe: write empty", "[io]") {
  pipe_reader r;
  pipe_writer w;
  std::tie(r, w) = make_pipe();
  auto t = std::thread{[&]() {
    w.write(std::span<uint8_t>{});
    w.close();
  }};
  uint8_t b[2];
  read_full(r, b);
  r.close();
  t.join();
}

TEST_CASE("Pipe: write after writer close", "[io]") {
  pipe_reader r;
  pipe_writer w;
  std::tie(r, w) = make_pipe();
  auto done = chan<std::error_code>{};
  std::error_code write_err;
  auto t = std::thread{[&]() {
    auto [_, err] = write_string(w, "hello");
    w.close();
    std::tie(_, write_err) = write_string(w, "world");
    done << err;
  }};
  auto buf = std::vector<uint8_t>(100);
  auto [n, err] = read_full(r, buf);
  CHECK((err == nil || err == error::unexpected_eof));
  err << done;
  CHECK(err == nil);
  CHECK(bytes::to_string(buf, n) == "hello");
  CHECK(write_err == error::closed_pipe);
  t.join();
}

TEST_CASE("Pipe: close error", "[io]") {
  auto test_error1 = std::make_error_code(std::errc::not_a_stream);
  auto test_error2 = std::make_error_code(std::errc::not_a_socket);

  pipe_reader r;
  pipe_writer w;
  std::tie(r, w) = make_pipe();
  r.close_with_error(test_error1);
  auto [_, err] = w.write(nil);
  CHECK(err == test_error1);
  r.close_with_error(test_error2);
  std::tie(_, err) = w.write(nil);
  CHECK(err == test_error1);

  std::tie(r, w) = make_pipe();
  w.close_with_error(test_error1);
  std::tie(_, err) = r.read(nil);
  CHECK(err == test_error1);
  w.close_with_error(test_error2);
  std::tie(_, err) = r.read(nil);
  CHECK(err == test_error1);
}

std::vector<uint8_t> sort_bytes_in_groups(std::span<uint8_t> b, int n) {
  std::vector<std::span<uint8_t>> groups;
  while (b.size() > 0) {
    groups.push_back(b.subspan(0, n));
    b = b.subspan(n);
  }
  std::sort(groups.begin(), groups.end(), [](std::span<uint8_t> a, std::span<uint8_t> b) { return bytes::less(a, b); });
  return bytes::join(groups, nil);
}

TEST_CASE("Pipe: concurrent", "[io]") {
  auto input = std::string_view{"0123456789abcdef"};
  auto count = 8;
  auto read_size = 2;
  SECTION("Write") {
    auto c = chan<pipe_return>{static_cast<size_t>(count)};
    auto threads = std::vector<std::thread>{};
    pipe_reader r;
    pipe_writer w;
    std::tie(r, w) = make_pipe();
    for (auto i = 0; i < count; ++i) {
      threads.push_back(std::thread{[&]() {
        std::this_thread::sleep_for(1ms);
        auto [n, err] = write_string(w, input);
        c << pipe_return{n, err};
      }});
    }

    auto buf = std::vector<uint8_t>(count*input.size());
    for (auto i = 0; i < static_cast<int>(buf.size()); i += read_size) {
      auto [n, err] = r.read(std::span{std::next(buf.begin(), i), std::next(buf.begin(), i+read_size)});
      CHECK(n == read_size);
      CHECK(err == nil);
    }

    CHECK(bytes::to_string(buf) == strings::repeat(input, count));
    for (auto& t : threads) {
      t.join();
    }
  }
  SECTION("Read") {
    struct result {
      int n;
      std::error_code err;
      std::vector<uint8_t> buf;
    };
    auto c = chan<result>{count*input.size()/read_size};
    auto threads = std::vector<std::thread>{};
    pipe_reader r;
    pipe_writer w;
    std::tie(r, w) = make_pipe();
    for (size_t i = 0; i < c.cap(); ++i) {
      threads.push_back(std::thread{[&]() {
        std::this_thread::sleep_for(1ms);
        auto buf = std::vector<uint8_t>(read_size);
        auto [n, err] = r.read(buf);
        c << result{n, err, std::move(buf)};
      }});
    }

    for (auto i = 0; i < count; ++i) {
      auto [n, err] = w.write(bytes::to_bytes(input));
      CHECK(n == static_cast<int>(input.size()));
      CHECK(err == nil);
    }

    auto got = std::vector<uint8_t>{};
    got.reserve(count*input.size());
    for (size_t i = 0; i < c.cap(); ++i) {
      result r;
      r << c;
      CHECK(r.n == read_size);
      CHECK(r.err == nil);
      got.insert(got.end(), r.buf.begin(), r.buf.end());
    }
    got = sort_bytes_in_groups(got, read_size);
    auto want = bytes::repeat(bytes::to_bytes(input), count);
    want = sort_bytes_in_groups(want, read_size);
    CHECK(bytes::to_string(got) == bytes::to_string(want));

    for (auto& t : threads) {
      t.join();
    }
  }
}

}  // namespace bongo::io
