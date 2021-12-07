/*
 * A close approximation of [channels][] (and [select][]) is implemented along
 * with most of the functional unit test suite. This example sends a message
 * over an unbuffered channel from one thread to another.
 *
 * [channels]: https://tour.golang.org/concurrency/2
 * [select]: https://tour.golang.org/concurrency/5
 */

#include <iostream>
#include <string>
#include <thread>

#include <bongo/bongo.h>

using namespace std::string_literals;

int main() try {
  bongo::chan<std::string> c;

  auto t = std::thread{[&]() {
    c << "Golang Rules!"s;
  }};

  decltype(c)::recv_type v;  // std::optional<std::string>
  v << c;
  std::cout << *v << "\n";

  t.join();
  return 0;
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
