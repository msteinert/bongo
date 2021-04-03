#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>

#include <bongo/sync.h>

using namespace std::chrono_literals;

int main() try {
  bongo::sync::wait_group wg;
  std::vector<std::thread> threads;

  for (int i = 0; i < 5; ++i) {
    wg.add(1);
    threads.emplace_back([&](int i) {
      printf("Worker %d starting\n", i);
      std::this_thread::sleep_for(1s);
      printf("Worker %d done\n", i);
      wg.done();
    }, i);
  }

  wg.wait();
  printf("All workers done\n");

  for (auto& t : threads) {
    t.join();
  }

  return 0;
} catch (std::exception const& e) {
  std::cerr << e.what() << "\n";
  return 1;
}
