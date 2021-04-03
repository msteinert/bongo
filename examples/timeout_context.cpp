#include <chrono>
#include <iostream>
#include <optional>
#include <variant>

#include <bongo/context.h>
#include <bongo/time.h>

using namespace std::chrono_literals;

int main() try {
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
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
