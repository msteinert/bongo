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
            << " seconds.\n";

  return 0;
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
