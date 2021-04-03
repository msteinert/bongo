#include <chrono>
#include <iostream>

#include <bongo/time.h>

using namespace std::chrono_literals;

int main() try {
  bongo::time::timer t{5s};

  decltype(t)::recv_type v;
  v << t.c();
  std::cout << "Timed out after "
            << std::chrono::duration_cast<std::chrono::seconds>(*v).count()
            << " seconds.\n";

  return 0;
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
