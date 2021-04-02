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
