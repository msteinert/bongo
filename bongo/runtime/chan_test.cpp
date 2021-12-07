// Copyright The Go Authors.

#include <atomic>
#include <chrono>
#include <cmath>
#include <map>
#include <thread>
#include <variant>
#include <vector>

#include <catch2/catch.hpp>

#include "bongo/runtime/chan.h"

using namespace std::chrono_literals;

namespace bongo::runtime {

TEST_CASE("Test channels", "[chan]") {
  size_t n = 20;
  for (size_t cap = 0; cap < n; ++cap) {
    CAPTURE(cap);

    {
      // Ensure receive from empty chan blocks
      auto c = chan<int>{cap};
      auto recv1 = false;
      auto t = std::thread{[&]() {
        int v;
        v << c;
        recv1 = true;
      }};
      auto recv2 = false;
      auto t2 = std::thread{[&]() {
        int v;
        v << c;
        recv2 = true;
      }};
      std::this_thread::sleep_for(1ms);
      REQUIRE(recv1 == false);
      REQUIRE(recv2 == false);
      // Ensure that non-blocking receive does not block
      std::optional<int> v;
      switch (select(
        recv_select_case(c, v),
        default_select_case()
      )) {
      case 0:
        FAIL_CHECK("receive from empty chan");
      default:
        break;
      }
      c << 0;
      c << 0;
      t.join();
      t2.join();
    }

    {
      // Ensure that send to a full chan blocks
      auto c = chan<int>{cap};
      for (int i = 0; i < static_cast<int>(cap); ++i) {
        c << i;
      }
      std::atomic_bool sent = false;
      auto t = std::thread([&]() {
        c << 0;
        sent.store(true);
      });
      std::this_thread::sleep_for(1ms);
      REQUIRE(sent.load() == false);
      // Ensure that non-blocking send does not block
      int v = 0;
      switch (select(
        send_select_case(c, std::move(v)),
        default_select_case()
      )) {
      case 0:
        FAIL_CHECK("send to full chan");
      default:
        break;
      }
      v << c;
      t.join();
    }

    {
      // Ensure that we receive 0 from closed chan
      auto c = chan<int>{cap};
      for (int i = 0; i < static_cast<int>(cap); ++i) {
        c << i;
      }
      c.close();
      for (int i = 0; i < static_cast<int>(cap); ++i) {
        int v;
        v << c;
        REQUIRE(v == i);
      }
      int v;
      v << c;
      REQUIRE(v == 0);
      std::optional<int> w;
      w << c;
      REQUIRE(w.has_value() == false);
    }

    {
      // Ensure that close unblocks receive
      auto c = chan<int>{cap};
      auto done  = chan<bool>();
      auto t = std::thread{[&]() {
        std::optional<int> v;
        v << c;
        done << (v.has_value() == false);
      }};
      std::this_thread::sleep_for(1ms);
      c.close();
      bool v;
      v << done;
      REQUIRE(v);
      t.join();
    }

    {
      // Send 100 integers,
      // ensure that we receive them non-corrupted in FIFO order
      auto c = chan<int>{cap};
      auto t = std::thread([&]() {
        for (int i = 0; i < 100; ++i) {
          c << i;
        }
      });
      for (int i = 0; i < 100; ++i) {
        int v;
        v << c;
        REQUIRE(v == i);
      }
      t.join();

      // Same, but using recv2
      t = std::thread([&]() {
        for (int i = 0; i < 100; ++i) {
          c << i;
        }
      });
      for (int i = 0; i < 100; ++i) {
        std::optional<int> v;
        v << c;
        REQUIRE(v.has_value());
        REQUIRE(v == i);
      }
      t.join();

      // Send 1000 integers in 4 threads,
      // ensure that we receive what we send
      int const P = 4;
      int const L = 1000;
      std::vector<std::thread> threads;
      for (int p = 0; p < P; ++p) {
        threads.emplace_back([&]() {
          for (int i = 0; i < L; ++i) {
            c << i;
          }
        });
      }
      auto done = chan<std::map<int, int>>{};
      for (int p = 0; p < P; ++p) {
        threads.emplace_back([&]() {
          auto recv = std::map<int, int>{};
          for (int i = 0; i < L; ++i) {
            int v;
            v << c;
            recv[v] = recv[v] + 1;
          }
          done << std::move(recv);
        });
      }
      auto recv = std::map<int, int>{};
      for (int p = 0; p < P; ++p) {
        auto value = std::map<int, int>{};
        value << done;
        for (auto [k, v] : value) {
          recv[k] = recv[k] + v;
        }
      }
      REQUIRE(recv.size() == L);
      for (auto p : recv) {
        REQUIRE(p.second == P);
      }
      for (auto& t : threads) {
        t.join();
      }
    }

    {
      // Test len/cap
      auto c = chan<int>{cap};
      REQUIRE(c.len() == 0);
      REQUIRE(c.cap() == cap);
      for (int i = 0; i < static_cast<int>(cap); ++i) {
        c << i;
      }
      REQUIRE(c.len() == cap);
      REQUIRE(c.cap() == cap);
    }
  }
}

TEST_CASE("Non-blocking receive race", "[chan]") {
  std::atomic_int failures = 0;
  int n = 1000;
  for (int i = 0; i < n; ++i) {
    auto c = chan<int>{1};
    c << 1;
    auto t = std::thread{[&]() {
      std::optional<int> v;
      switch (select(
        recv_select_case(c, v),
        default_select_case()
      )) {
      case 0:
        break;
      default:
        ++failures;
        break;
      }
    }};
    c.close();
    int v;
    v << c;
    REQUIRE(failures == 0);
    t.join();
  }
}

TEST_CASE("Non-blocking select race2", "[chan]") {
  int n = 1000;
  auto done = chan<bool>{1};
  for (int i = 0; i < n; ++i) {
    auto c1 = chan<int>{1};
    auto c2 = chan<int>{};
    c1 << 1;
    auto t = std::thread{[&]() {
      std::optional<int> v1, v2;
      switch (select(
        recv_select_case(c1, v1),
        recv_select_case(c2, v2),
        default_select_case()
      )) {
      case 0:
      case 1:
        break;
      default:
        done << false;
        return;
      }
      done << true;
    }};
    c2.close();
    std::optional<int> v;
    switch (select(
      recv_select_case(c1, v),
      default_select_case()
    )) {
    default:
      break;
    }
    bool d;
    d << done;
    REQUIRE(d);
    t.join();
  }
}

TEST_CASE("Self select", "[chan]") {
  // Ensure send/recv on the same chan in select does not crash nor deadlock
  for (size_t cap : std::vector<size_t>{0, 10}) {
    std::atomic_bool error = false;
    auto threads = std::vector<std::thread>{};
    auto c = chan<int>{cap};
    for (int p = 0; p < 2; ++p) {
      threads.emplace_back([&, cap, p]() {
        for (auto i = 0; i < 10; ++i) {
          if ((p == 0) || (i%2 == 0)) {
            int v0 = p;
            std::optional<int> v1;
            switch (select(
              send_select_case(c, std::move(v0)),
              recv_select_case(c, v1)
            )) {
            case 0:
              break;
            case 1:
              if (cap == 0 && v1 == p) {
                error = true;
                return;
              }
              break;
            }
          } else {
            std::optional<int> v0;
            int v1 = p;
            switch (select(
              recv_select_case(c, v0),
              send_select_case(c, std::move(v1))
            )) {
            case 0:
              if (cap == 0 && v0 == p) {
                error = true;
                return;
              }
              break;
            case 1:
              break;
            }
          }
        }
      });
    }
    for (auto& t : threads) {
      t.join();
    }
    REQUIRE(error == false);
  }
}

TEST_CASE("Select stress", "[chan]") {
  std::vector<std::thread> threads;
  auto c = std::array<chan<int>, 4>{
    chan<int>{},
    chan<int>{},
    chan<int>{2},
    chan<int>{3}
  };
  int N = 10000;
  for (int k = 0; k < 4; ++k) {
    threads.emplace_back([&, N, k]() {
      for (int i = 0; i < N; ++i) {
        c[k] << 0;
      }
    });
    threads.emplace_back([&, N, k]() {
      for (int i = 0; i < N; ++i) {
        int v;
        v << c[k];
      }
    });
  }
  threads.emplace_back([&, N]() {
    int n[4]{0};
    int v0 = 0, v1 = 0, v2 = 0, v3 = 0;
    select_case cases[] = {
      send_select_case(c[3], std::move(v3)),
      send_select_case(c[2], std::move(v2)),
      send_select_case(c[0], std::move(v0)),
      send_select_case(c[1], std::move(v1)),
    };
    for (int i = 0; i < 4*N; ++i) {
      switch (select(cases)) {
      case 0:
        ++n[3];
        if (n[3] == N) {
          cases[0].chan = nullptr;
        } else {
          v3 = std::move(0);
        }
        break;
      case 1:
        ++n[2];
        if (n[2] == N) {
          cases[1].chan = nullptr;
        } else {
          v2 = std::move(0);
        }
        break;
      case 2:
        ++n[0];
        if (n[0] == N) {
          cases[2].chan = nullptr;
        } else {
          v0 = std::move(0);
        }
        break;
      case 3:
        ++n[1];
        if (n[1] == N) {
          cases[3].chan = nullptr;
        } else {
          v1 = std::move(0);
        }
        break;
      }
    }
  });
  threads.emplace_back([&, N]() {
    int n[4]{0};
    std::optional<int> v0 = 0, v1 = 0, v2 = 0, v3 = 0;
    select_case cases[] = {
      recv_select_case(c[0], v0),
      recv_select_case(c[1], v1),
      recv_select_case(c[2], v2),
      recv_select_case(c[3], v3),
    };
    for (int i = 0; i < 4*N; ++i) {
      switch (select(cases)) {
      case 0:
        ++n[0];
        if (n[0] == N) {
          cases[0].chan = nullptr;
        }
        break;
      case 1:
        ++n[1];
        if (n[1] == N) {
          cases[1].chan = nullptr;
        }
        break;
      case 2:
        ++n[2];
        if (n[2] == N) {
          cases[2].chan = nullptr;
        }
        break;
      case 3:
        ++n[3];
        if (n[3] == N) {
          cases[3].chan = nullptr;
        }
        break;
      }
    }
  });
  for (auto& t : threads) {
    t.join();
  }
  SUCCEED("did not deadlock");
}

TEST_CASE("Select fairness", "[chan]") {
  int const trials = 10000;
  auto c1 = chan<int>{trials + 1};
  auto c2 = chan<int>{trials + 1};
  for (int i = 0; i < trials; ++i) {
    c1 << 1;
    c2 << 2;
  }
  auto c3 = chan<int>{};
  auto c4 = chan<int>{};
  auto out = chan<int>{};
  auto done = chan<int>{};
  auto t = std::thread{[&]() {
    while (true) {
      std::optional<int> b;
      switch (select(
        recv_select_case(c3, b),
        recv_select_case(c4, b),
        recv_select_case(c1, b),
        recv_select_case(c2, b)
      )) {
      default:
        break;
      }
      std::optional<int> d;
      switch (select(
        send_select_case(out, std::move(b.value())),
        recv_select_case(done, d)
      )) {
      case 0:
        break;
      case 1:
        return;
      }
    }
  }};
  int cnt1 = 0, cnt2 = 0;
  for (int i = 0; i < trials; ++i) {
    int b;
    b << out;
    switch (b) {
    case 1:
      ++cnt1;
      break;
    case 2:
      ++cnt2;
      break;
    default:
      CAPTURE(cnt1, cnt2);
      FAIL_CHECK("unexpected value on channel");
    }
  }
  auto r = (double)cnt1 / trials;
  auto e = std::abs(r - 0.5);
  if (e > 4.891676/(2*std::sqrt(trials))) {
    CAPTURE(cnt1, cnt2, r, e);
    FAIL_CHECK("unfair select");
    t.join();
  }
  SUCCEED("select was fair");
  done.close();
  t.join();
}

TEST_CASE("Pseudo-random send", "[chan]") {
  size_t n = 100;
  for (size_t cap : std::vector<size_t>{0, n}) {
    auto c = chan<int>{cap};
    auto l = std::vector<int>{};
    l.resize(n, 0);
    auto t = std::thread{[&]() {
      for (size_t i = 0; i < n; ++i) {
        std::this_thread::yield();
        l[i] << c;
      }
    }};
    for (size_t i = 0; i < n; ++i) {
      int b0 = 0, b1 = 1;
      switch (select(
        send_select_case(c, std::move(b1)),
        send_select_case(c, std::move(b0))
      )) {
      default:
        break;
      }
    }
    t.join();
    int n0 = 0, n1 = 0;
    for (auto i : l) {
      n0 += (i + 1) % 2;
      n1 += i;
    }
    if (n0 <= (int)n/10 || n1 <= (int)n/10) {
      CAPTURE(n0, n1, cap);
      FAIL_CHECK("wanted pseudo-random");
    }
    SUCCEED("send was pseudo-random");
  }
}

TEST_CASE("Multi-consumer", "[chan]") {
  int const nwork = 23;
  int const niter = 271828;
  auto const pn = std::vector<int>{2, 3, 7, 11, 13, 17, 19, 23, 27, 31};
  auto q = chan<int>{nwork * 3};
  auto r = chan<int>{nwork * 3};

  // Workers
  auto workers = std::vector<std::thread>{};
  for (int i = 0; i < nwork; ++i) {
    workers.emplace_back([&, i]() {
      for (auto v : q) {
        if (pn[i % pn.size()] == v) {
          std::this_thread::yield();
        }
        r << v;
      }
    });
  }

  // Feeder & closer
  int expect = 0;
  auto t = std::thread{[&]() {
    for (int i = 0; i < niter; ++i) {
      int v = pn[i % pn.size()];
      expect += v;
      q << v;
    }
    q.close();
    for (auto& t : workers) {
      t.join();
    }
    r.close();
  }};

  // Consume & check
  int n = 0;
  int s = 0;
  for (auto v : r) {
    ++n;
    s += v;
  }
  t.join();
  REQUIRE(n == niter);
  REQUIRE(s == expect);
}

TEST_CASE("Select duplicate channel", "[chan]") {
  auto c = chan<int>{};
  auto d = chan<int>{};
  auto e = chan<int>{};

  auto t1 = std::thread{[&]() {
    std::optional<int> v;
    switch (select(
      recv_select_case(c, v),
      recv_select_case(c, v),
      recv_select_case(d, v)
    )) {
    default:
      break;
    }
    e << 9;
  }};
  std::this_thread::sleep_for(1ms);

  auto t2 = std::thread{[&]() {
    int v;
    v << c;
  }};
  std::this_thread::sleep_for(1ms);

  int v;
  d << 7;  // Wakes up t1
  v << e;  // Tells us t1 is done
  c << 8;  // Wakes up t2

  t1.join();
  t2.join();
}

#if defined(CATCH_CONFIG_ENABLE_BENCHMARKING)

TEST_CASE("Channel benchmarks", "[!benchmark]") {
  BENCHMARK("Construct std::byte") {
    auto c = chan<std::byte>{8};
  };

  BENCHMARK("Construct int") {
    auto c = chan<int>{8};
  };

  BENCHMARK("Construct std::byte*") {
    auto c = chan<std::byte*>{8};
  };

  BENCHMARK("Construct std::monostate") {
    auto c = chan<std::monostate>{8};
  };

  using struct32 = struct { int64_t a, b, c, d; };
  static_assert(sizeof (struct32) == 32);
  BENCHMARK("Construct struct32") {
    auto c = chan<struct32>{8};
  };

  using struct40 = struct { int64_t a, b, c, d, e; };
  static_assert(sizeof (struct32) == 32);
  BENCHMARK("Construct struct40") {
    auto c = chan<struct40>{8};
  };

  BENCHMARK_ADVANCED("Non-blocking select")(Catch::Benchmark::Chronometer meter) {
    auto c = chan<int>{};
    meter.measure([&]() {
      std::optional<int> v;
      switch (select(
        recv_select_case(c, v),
        default_select_case()
      )) {
      default:
        break;
      }
    });
  };

  BENCHMARK_ADVANCED("Uncontended select")(Catch::Benchmark::Chronometer meter) {
    auto c1 = chan<int>{1};
    auto c2 = chan<int>{1};
    c1 << 0;
    meter.measure([&]() {
      std::optional<int> v;
      switch (select(
        recv_select_case(c1, v),
        recv_select_case(c2, v)
      )) {
      case 0:
        c2 << 0;
        break;
      case 1:
        c1 << 0;
        break;
      }
    });
  };

  BENCHMARK_ADVANCED("Contended synchronous select")(Catch::Benchmark::Chronometer meter) {
    auto c1 = chan<int>{};
    auto c2 = chan<int>{};
    auto c3 = chan<int>{};
    auto done = chan<int>{};
    auto t = std::thread([&]() {
      while (true) {
        int v1 = 0, v2 = 0, v3 = 0;
        std::optional<int> d;
        switch (select(
          send_select_case(c1, std::move(v1)),
          send_select_case(c2, std::move(v2)),
          send_select_case(c3, std::move(v3)),
          recv_select_case(done, d)
        )) {
        case 0:
        case 1:
        case 2:
          break;
        case 3:
          return;
        }
      }
    });
    meter.measure([&]() {
      std::optional<int> v;
      switch (select(
        recv_select_case(c1, v),
        recv_select_case(c2, v),
        recv_select_case(c3, v)
      )) {
      default:
        break;
      }
    });
    done.close();
    t.join();
  };

  BENCHMARK_ADVANCED("Contended asynchronous select")(Catch::Benchmark::Chronometer meter) {
    auto n = std::thread::hardware_concurrency();
    auto c1 = chan<int>{n};
    auto c2 = chan<int>{n};
    c1 << 0;
    meter.measure([&]() {
      std::optional<int> v;
      switch (select(
        recv_select_case(c1, v),
        recv_select_case(c2, v)
      )) {
      case 0:
        c2 << 0;
        break;
      case 1:
        c1 << 0;
        break;
      }
    });
  };

  BENCHMARK_ADVANCED("Multiple non-blocking select")(Catch::Benchmark::Chronometer meter) {
    auto c1 = chan<int>{};
    auto c2 = chan<int>{};
    auto c3 = chan<int>{1};
    auto c4 = chan<int>{1};
    meter.measure([&]() {
      int send1 = 0, send2 = 0;
      std::optional<int> recv;
      switch (select(
        recv_select_case(c1, recv),
        default_select_case()
      )) {
      default:
        break;
      }
      switch (select(
        send_select_case(c2, std::move(send1)),
        default_select_case()
      )) {
      default:
        break;
      }
      switch (select(
        recv_select_case(c1, recv),
        default_select_case()
      )) {
      default:
        break;
      }
      switch (select(
        send_select_case(c1, std::move(send2)),
        default_select_case()
      )) {
      default:
        break;
      }
    });
  };

  BENCHMARK_ADVANCED("Uncontended")(Catch::Benchmark::Chronometer meter) {
    int const n = 100;
    auto c = chan<int>{n};
    meter.measure([&]() {
      for (int i = 0; i < n; ++i) {
        c << 0;
      }
      int v;
      for (int i = 0; i < n; ++i) {
        v << c;
      }
    });
  };

  BENCHMARK_ADVANCED("Sem")(Catch::Benchmark::Chronometer meter) {
    auto c = chan<std::monostate>{1};
    std::optional<std::monostate> v;
    meter.measure([&]() {
      c << std::monostate{};
      v << c;
    });
  };

  BENCHMARK_ADVANCED("Popular")(Catch::Benchmark::Chronometer meter) {
    int const n = 1000;
    auto c = chan<bool>{};
    auto done = chan<bool>{};

    auto channels = std::vector<std::unique_ptr<chan<bool>>>{};
    auto threads = std::vector<std::thread>{};

    for (int i = 0; i < n; ++i) {
      channels.push_back(std::make_unique<chan<bool>>());
      threads.emplace_back([&](chan<bool>* d) {
        while (true) {
          std::optional<bool> v;
          switch (select(
            recv_select_case(c, v),
            recv_select_case(d, v),
            recv_select_case(done, v)
          )) {
          case 0:
          case 1:
            break;
          case 2:
            return;
          }
        }
      }, channels[i].get());
    }

    meter.measure([&](int n) {
      auto m = (n+1) * 10;
      for (int i = 0; i < m; ++i) {
        for (auto& d : channels) {
          *d << true;
        }
      }
    });

    done.close();
    for (auto& t : threads) {
      t.join();
    }
  };

  BENCHMARK_ADVANCED("Select on closed")(Catch::Benchmark::Chronometer meter) {
    auto c = chan<std::monostate>{};
    c.close();
    meter.measure([&]() {
      std::optional<std::monostate> v;
      switch (select(
        recv_select_case(c, v),
        default_select_case()
      )) {
      case 0:
        break;
      default:
        FAIL_CHECK("unreachable");
      }
    });
  };
}

#endif  // defined(CATCH_CONFIG_ENABLE_BENCHMARKING)

}  // namespace bongo::runtime
