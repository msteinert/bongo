#include <iostream>
#include <string>
#include <thread>

#include <bongo/chan.h>

using namespace std::string_literals;

int main() {
  bongo::chan<std::string> c;

  auto t = std::thread{[&]() {
    c << "Golang Rules!"s;
  }};

  decltype(c)::recv_type v;  // std::optional<std::string>
  v << c;
  std::cout << *v << "\n";

  t.join();
  return 0;
}
