/*
 * The [context][] package is implemented. This includes four usable context
 * patterns via the following factory functions. The following example shows
 * how to use a cancel context to signal that a thread should exit.
 *
 * [context]: https://golang.org/pkg/context/
 */

#include <iostream>
#include <memory>
#include <thread>
#include <utility>

#include <bongo/bongo.h>
#include <bongo/context.h>

int main() try {
  auto [ctx, cancel] = bongo::context::with_cancel(bongo::context::background());
  bongo::chan<long> dst;

  // This thread generates integers and sends them to a channel. The main thread
  // needs to cancel the context once it is done consuming generated integers so
  // the thread will exit.
  std::thread t{[&](std::shared_ptr<bongo::context::context> ctx) {
    long n = 1;
    while (true) {
      std::optional<std::monostate> done;
      long v = n;
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
  return 0;
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
