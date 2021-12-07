/*
 * A value context associates a key with a value. Value context should only be
 * used to store request-scoped data.
 */

#include <any>
#include <iostream>
#include <memory>
#include <string>

#include <bongo/context.h>

using namespace std::string_literals;

int main() try {
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
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
