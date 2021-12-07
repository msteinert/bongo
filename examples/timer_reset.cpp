/*
 * Timers may be reset, but only after they have been stopped or timed out and
 * the channel is drained.
 *
 * One major difference between this implementation and the Go implementation
 * is how `timer.AfterFunc()` is emulated. In the Go implementation the
 * callback occurs in a separate goroutine. In this package the callback
 * function is called in the timer thread.
 */

#include <chrono>
#include <iostream>

#include <bongo/time.h>

using namespace std::chrono_literals;

int main() try {
  bongo::time::timer t{100ms};
  decltype(t)::recv_type v;

  if (!t.stop()) {
    v << t.c();
  }

  t.reset(50ms);
  v << t.c();
  std::cout << "Timed out after "
            << std::chrono::duration_cast<std::chrono::milliseconds>(*v).count()
            << " milliseconds.\n";

  return 0;
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
