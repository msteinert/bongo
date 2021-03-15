# Bongo

![Baby Bongo](share/images/baby-bongo.jpg)

---

Bongo is a port of some APIs from [Go][] to C++. This code is heavily based
on the [Go source][] and is therefore covered by the same copyright and
3-clause BSD [license][].

[Go]: https://golang.org
[Go source]: https://github.com/golang/go
[license]: LICENSE

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
  bongo::chan<std::sring> c;

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

Select is implemented in a similar fashion as [Select][] found in the
[reflect][] package:

```cpp
#include <iostream>
#include <string>
#include <thread>

#include <bongo/chan.h>

using namespace std::string_literals;

int main() {
  bongo::chan<std::sring> c0;
  bongo::chan<std::sring> c1;

  auto t0 = std::thread{[&]() {
    c0 << "one"s;
  }};
  auto t1 = std::thread{[&]() {
    c1 << "two"s;
  }};

  for (int i = 0; i < 2; ++i) {
    std::optional<std::string>::recv_type msg0, msg1;
    switch (bongo::select({
      bongo::recv_select_case(c0, msg0),
      bongo::recv_select_case(c1, msg1),
    })) {
    case 0:
      std::cout << "received: " << *msg0 << "\n";
      break;
    case 1:
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
[Select]: https://golang.org/pkg/reflect/#Select
[reflect]:  https://golang.org/pkg/reflect/

### Context

The [context] package is implemented. This includes four usable context
patterns via the following factory functions:

- `bongo::context::with_cancel(parent)`
- `bongo::context::with_timeout(parent, duration)`
- `bongo::context::with_deadline(parent, time_point)`
- `bongo::context::with_value(parent, key, value)`

The background and TODO contexts are also available:

- `bongo::context::background()`
- `bongo::context::todo()`

[context]: https://golang.org/pkg/context/

### Sync

The [WaitGroup][] type from the [sync][] package is implemented. This is a
naive implementation using a mutex and a condition variable. A possible
future improvement might use a lockless algorithm.

```
#include <chrono>
#include <iostream>
#include <thread>

#include <bongo/sync.h>
#include <fmt/format.h>

using std::chrono_literals;

int main() {
  bongo::wait_group wg;
  std::thread threads;

  for (int i = 0; i < 5; ++i) {
    wg.add();
    threads.emplace_back([&](int i) {
      std::cout << fmt::format("Worker {} starting\n", i);
      std::this_thread::sleep_for(1sec);
      std::cout << fmt::format("Worker {} done\n", i);
      wg.done();
    }(i));
  }

  wg.wait();
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

using std::chrono_literals;

int main() {
  bongo::time::timer t{1sec};

  decltype(t)::recv_type v;  // std::optional<std::chrono::seconds>
  v << t.c();
  std::cout << "Timed out after " << v->count() << " seconds.\n";

  return 0;
}
```

Timers may be reset, but only after they have been stopped or timed out and
the channel is drained:

```cpp
#include <chrono>
#include <iostream>

#include <bongo/time.h>

using std::chrono_literals;

int main() {
  bongo::time::timer t{100ms};
  decltype(t)::recv_type v;

  if (!t.stop()) {
    v << t.c();
  }

  t.reset(50ms);
  v << t.c();
  std::cout << "Timed out after " << v->count() << " seconds.\n";

  return 0;
}
```

One major difference between this implementation and the Go implementation
is how `timer.AfterFunc()` is emulated. In the Go implementation the
callback occurs in a separate goroutine. In this package the callback
function is called in the timer thread. This means that the timer itself
cannot be manipulated via `bongo::timer::stop()` or `bongo::timer::reset()`
from the callback function. If you need functionality similar to the Go
package it can be implemented by managing your own thread that listens to
the timeout channel.


[Timer]: https://golang.org/pkg/time/#Timer
[time]: https://golang.org/pkg/time/
