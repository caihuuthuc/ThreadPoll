#include "../thread_pool.hpp"
#include <list>
#include <iostream>
#include <numeric>

template <typename T, typename Iterator = typename std::list<T>::iterator>
struct accumulation_block {
  T operator() (Iterator begin, Iterator end) {
    return std::accumulation(begin, end, T{});
  }
};

template <typename T, typename Iterator = typename std::list<T>::iterator>
T accumulation(Iterator first, Iterator last, T init) {
  thread_pool pool(2);
  unsigned long const length = std::distance(first, last);
  if (!length) return init;
  unsigned long const block_size = 25;
  unsigned long const num_blocks = (length + block_size - 1) /block_size;
  
  std::vector<std::future<T>> futures;
  Iterator block_begin = first;
  
  for (auto i = 0; i < num_blocks - 1; ++i) {
    Iterator block_end = block_begin;
    std::advance(block_end, block_size);
    futures.push_back(pool.submit(accumulation_block<T>(), block_begin, block_end));
    block_begin = block_end;
  }
  futures.push_back(pool.submit(accumulation_block<T>(), block_begin, last));
  T result = init;
  for (auto i =0; i < num_blocks; ++i) {
    result += futures[i].get();
  }
}

int main() {
  std::list<int> intList {1,2,3,4,5,6,7,8,9};
  auto result = accumulation<int>(intList.begin(), intList.end(), 0);
  std::cout << result << "\n";
  
}
