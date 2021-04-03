#include <iostream>
#include <string>
#include <thread>

#include <bongo/chan.h>

using namespace std::string_literals;

int main() try {
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
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
