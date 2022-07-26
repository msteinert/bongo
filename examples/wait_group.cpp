/*
 * The [WaitGroup][] type from the [sync][] package is implemented. This is a
 * naive implementation using a mutex and a condition variable. A possible
 * future improvement might use a lockless algorithm.
 *
 * This example is somewhat pointless since joining the threads accomplishes
 * the same result, however it illustrates how a wait group might be used, for
 * example, to wait for results from a thread pool.
 *
 * [WaitGroup]: https://golang.org/pkg/sync/#WaitGroup
 * [sync]: https://golang.org/pkg/sync/
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <bongo/sync.h>

using namespace std::chrono_literals;

int main() try {
  bongo::sync::wait_group wg;
  std::vector<std::thread> threads;

  for (long i = 0; i < 5; ++i) {
    wg.add(1);
    threads.emplace_back([&](long i) {
      std::cout << "Worker " << i << " starting\n";
      std::this_thread::sleep_for(1s);
      std::cout << "Worker " << i << " done\n";
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
