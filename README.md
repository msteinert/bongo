# Bongo

![Baby Bongo](share/images/baby-bongo.jpg)

---

Bongo is a port of some concurrency APIs from [Go][] to C++. This code is
heavily based on the [Go source][] and is therefore covered by the same
copyright and 3-clause BSD [license][].

[Go]: https://golang.org
[Go source]: https://github.com/golang/go
[license]: LICENSE

## Contents

- [API](#api)
- [Development](#development)
- [Versioning](#versioning)

## API

The author found himself missing some of the nice features provided by Go
when developing C++ applications. This is the result of an effort to fill in
those gaps. Take it as you will.

### Channels

A close approximation of [channels][] (and [select][]) is implemented along
with most of the functional unit test suite. The following example sends a
message over an unbuffered channel from one thread to another:

```cpp
#include <iostream>
#include <string>
#include <thread>

#include <bongo/chan.h>

using namespace std::string_literals;

int main() {
  bongo::chan<std::string> c;

  auto t = std::thread{[&]() {
    c << "Golang Rules!"s;
  }};

  decltype(c)::recv_type v;  // std::optional<std::string>
  v << c;
  std::cout << *v << "\n";

  t.join();
  return 0;
}
```

Select is implemented in a similar fashion as [Select][reflect.Select] found
in the [reflect][] package:

```cpp
#include <iostream>
#include <string>
#include <thread>

#include <bongo/chan.h>

using namespace std::string_literals;

int main() {
  bongo::chan<std::string> c0, c1;

  auto t0 = std::thread{[&]() {
    c0 << "one"s;
  }};
  auto t1 = std::thread{[&]() {
    c1 << "two"s;
  }};

  for (int i = 0; i < 2; ++i) {
    std::optional<std::string> msg0, msg1;
    bongo::select_case cases[] = {
      bongo::recv_select_case(c0, msg0),
      bongo::recv_select_case(c1, msg1),
    };
    switch (bongo::select(cases)) {
    case 0:  // Value corresponds with index 0 in "cases"
      std::cout << "received: " << *msg0 << "\n";
      break;
    case 1:  // Value corresponds with index 1 in "cases"
      std::cout << "received: " << *msg1 << "\n";
      break;
    }
  }

  t0.join();
  t1.join();
  return 0;
}
```

[channels]: https://tour.golang.org/concurrency/2
[select]: https://tour.golang.org/concurrency/5
[reflect.Select]: https://golang.org/pkg/reflect/#Select
[reflect]:  https://golang.org/pkg/reflect/

### Context

The [context] package is implemented. This includes four usable context
patterns via the following factory functions. The following examples shows
how to use a cancel context to signal that a thread should exit.

```cpp
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

#include <bongo/chan.h>
#include <bongo/context.h>

int main() {
  auto [ctx, cancel] = bongo::context::with_cancel(bongo::context::background());
  bongo::chan<int> dst;

  // This thread generates integers and sends them to a channel. The main thread
  // needs to cancel the context once it is done consuming generated integers so
  // the thread will exit.
  std::thread t{[&](std::shared_ptr<bongo::context::context> ctx) {
    auto n = 1;
    while (true) {
      std::optional<std::monostate> done;
      auto v = n;
      switch (bongo::select(
        bongo::recv_select_case(ctx->done(), done),
        bongo::send_select_case(dst, std::move(v))
      )) {
      case 0:
        return;  // Main thread is finished processing
      case 1:
        ++n;
        break;
      }
    }
  }, ctx};

  for (auto n : dst) {
    std::cout << n << "\n";
    if (n == 5) {
      break;
    }
  }

  cancel();
  t.join();
}
```

The function `bongo::context::with_timeout` returns a cancelable context
that is automatically canceled after the specified duration.

```cpp
#include <chrono>
#include <iostream>
#include <optional>
#include <variant>

#include <bongo/context.h>
#include <bongo/time.h>

using namespace std::chrono_literals;

int main() {
  // Pass a context with a timeout to tell a blocking function that should
  // abandon its work after the timeout elapses.
  auto [ctx, cancel] = bongo::context::with_timeout(bongo::context::background(), 1ms);
  auto t = bongo::time::timer{1s};

  decltype(t)::recv_type v0;
  std::optional<std::monostate> v1;
  switch (bongo::select(
    bongo::recv_select_case(t.c(), v0),
    bongo::recv_select_case(ctx->done(), v1)
  )) {
  case 0:
    std::cout << "overslept\n";
    break;
  case 1:
    try {
      std::rethrow_exception(ctx->err());
    } catch (std::exception const& e) {
      std::cout << e.what() << "\n";  // prints "context deadline exceeded"
    }
    break;
  }

  cancel();
  return 0;
}
```

The function `bongo::context::with_deadline` returns a cancelable context
that is automatically canceled at the specified point in time.

```cpp
#include <chrono>
#include <iostream>
#include <optional>
#include <variant>

#include <bongo/context.h>
#include <bongo/time.h>

using namespace std::chrono_literals;

int main() {
  auto d = std::chrono::system_clock::now() + 1ms;
  auto [ctx, cancel] = bongo::context::with_deadline(bongo::context::background(), d);
  auto t = bongo::time::timer{1s};

  decltype(t)::recv_type v0;
  std::optional<std::monostate> v1;
  switch (bongo::select(
    bongo::recv_select_case(t.c(), v0),
    bongo::recv_select_case(ctx->done(), v1)
  )) {
  case 0:
    std::cout << "overslept\n";
    break;
  case 1:
    try {
      std::rethrow_exception(ctx->err());
    } catch (std::exception const& e) {
      std::cout << e.what() << "\n";
    }
    break;
  }

  // Even though ctx will be expired, it is good practice to call its
  // cancellation function in any case. Failure to do so may keep the context
  // and its parent alive longer than necessary.
  cancel();
  return 0;
}
```

A value context associates a key with a value. Value context should only be
used to store request-scoped data.

```cpp
#include <any>
#include <memory>
#include <iostream>
#include <string>

#include <bongo/context.h>

using namespace std::string_literals;

int main() {
  auto f = [](std::shared_ptr<bongo::context::context> ctx, std::string const& k) {
    auto v = ctx->value(k);
    if (v.has_value()) {
      std::cout << k << ": " << std::any_cast<std::string>(v) << "\n";
      return;
    }
    std::cout << "key not found: " << k << "\n";
  };

  auto k = std::string{"language"};
  auto ctx = bongo::context::with_value(bongo::context::background(), k, "Go"s);

  f(ctx, k);
  f(ctx, std::string{"color"});

  return 0;
}
```

[context]: https://golang.org/pkg/context/

### Sync

The [WaitGroup][] type from the [sync][] package is implemented. This is a
naive implementation using a mutex and a condition variable. A possible
future improvement might use a lockless algorithm.

```cpp
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

#include <bongo/sync.h>

using namespace std::chrono_literals;

int main() {
  bongo::sync::wait_group wg;
  std::vector<std::thread> threads;

  for (int i = 0; i < 5; ++i) {
    wg.add(1);
    threads.emplace_back([&](int i) {
      printf("Worker %d starting\n", i);
      std::this_thread::sleep_for(1s);
      printf("Worker %d done\n", i);
      wg.done();
    }, i);
  }

  wg.wait();
  printf("All workers done\n");

  for (auto& t : threads) {
    t.join();
  }

  return 0;
}
```

This example is somewhat pointless since joining the threads accomplishes
the same result, however it illustrates how a wait group might be used, for
example, to wait for results from a thread pool.

[WaitGroup]: https://golang.org/pkg/sync/#WaitGroup
[sync]: https://golang.org/pkg/sync/

### Time

The [Timer][] type from the [time][] package is implemented. This type was
implemented in order to build the context classes. Timers operate by
spawning a thread that waits on a condition variable. If the condition is
notified (via `bongo::timer::stop()`) the timer is canceled, otherwise it
times out, e.g.:

```cpp
#include <chrono>
#include <iostream>

#include <bongo/time.h>

using namespace std::chrono_literals;

int main() {
  bongo::time::timer t{5s};

  decltype(t)::recv_type v;
  v << t.c();
  std::cout << "Timed out after "
            << std::chrono::duration_cast<std::chrono::seconds>(*v).count()
            << " seconds.\n";

  return 0;
}
```

Timers may be reset, but only after they have been stopped or timed out and
the channel is drained:

```cpp
#include <chrono>
#include <iostream>

#include <bongo/time.h>

using namespace std::chrono_literals;

int main() {
  bongo::time::timer t{100ms};
  decltype(t)::recv_type v;

  if (!t.stop()) {
    v << t.c();
  }

  t.reset(50ms);
  v << t.c();
  std::cout << "Timed out after "
            << std::chrono::duration_cast<std::chrono::milliseconds>(*v).count()
            << " seconds.\n";

  return 0;
}
```

One major difference between this implementation and the Go implementation
is how `timer.AfterFunc()` is emulated. In the Go implementation the
callback occurs in a separate goroutine. In this package the callback
function is called in the timer thread.

[Timer]: https://golang.org/pkg/time/#Timer
[time]: https://golang.org/pkg/time/

## Development

CMake and a C++17 compiler are required to build the project. The test suite
requires [Catch2][]. To build with devtoolset-7 using Conan something
similar to the following should build the project and run the test suite:

```
$ mkdir build && cd build
$ . /opt/rh/devtoolset-7/enable
$ conan install -pr gcc-7 ..
$ cmake ..
$ ninja
$ ./tests/bongo-test
```

To run the benchmarks:

```
./tests/bongo-test "~[benchmark]"
```

[Catch2]: https://github.com/catchorg/Catch2

## Versioning

A [semantic version][] number is computed based on a git tag when packages
are generated. The major and minor version numbers come from a tag. The
patch version is computed as the number of commits since the last version
tag. The CI job publishes `Release` and `Debug` packages to Artifactory.

[semantic version]: https://semver.org/
