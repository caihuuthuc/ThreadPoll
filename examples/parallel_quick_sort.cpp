#include <list>
#include <iostream>
#include <algorithm>
#include <future>
#include <thread>

#include "../ThreadPool.hpp"

ThreadPool pool(2);

std::ostream& operator<<(std::ostream& ostr, const std::list<int>& list)
{
    for (auto& i : list)
        ostr << ' ' << i;

    return ostr;
}

template <typename T, typename Iterator = typename std::list<T>::iterator>
struct parallel_quick_sort_impl {
	
	[[nodiscard]] bool is_sorted(const std::list<T> & chunk_data) {
		if (chunk_data.size() == 1) return true;
		auto previous_item = chunk_data.cbegin();
		auto next_item = previous_item;
		do {
			std::advance(next_item, 1);
			if (*previous_item > *next_item) return false;
		} while (next_item != chunk_data.cend());
		return true;
	}

	[[nodiscard]] std::list<T> do_sort(std::list<T> & chunk_data) {
		if (chunk_data.empty() || is_sorted(chunk_data)) return chunk_data;
		std::list<T> result;
		result.splice(result.begin(), chunk_data, chunk_data.begin());
		T const& partition_val = *result.begin();
		Iterator divide_point = std::partition(chunk_data.begin(), chunk_data.end(), [&](T const & val) {return val < partition_val;});

		// handle lower chunk
		std::list<T> lower_chunk_data;
		lower_chunk_data.splice(lower_chunk_data.begin(), chunk_data, chunk_data.begin(), divide_point);
		std::cout << "lower_chunk_data: " << lower_chunk_data << "\n";
		auto lower_chunk_data_future = pool.submit(std::bind(&parallel_quick_sort_impl::do_sort, this, std::ref(lower_chunk_data)));
		
		// handle higher chunk
		std::list<T> sorted_higher_chunk_data (do_sort(chunk_data));
		result.splice(result.end(), sorted_higher_chunk_data);

		// merge lower and higher into result and return
		while(lower_chunk_data_future.wait_for(std::chrono::seconds(0)) == std::future_status::timeout) {
			pool.run_pending_task();
		}
		std::list<T> sorted_lower_chunk_data = lower_chunk_data_future.get();

		result.splice(result.begin(), sorted_lower_chunk_data);

		return result;
	}		
};

template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input) {
	if (input.empty()) return input;
	parallel_quick_sort_impl<T> impl;
	return impl.do_sort(input);
}


int main() {
	std::list<int> intList {106,303, 102,36,41,15,96,};
	std::cout << "intList: " << intList << "\n";
	auto result = parallel_quick_sort<int>(intList);

	std::cout << "Run thread pool quicksort: " << result << "\n";	

}
