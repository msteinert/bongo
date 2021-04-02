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
