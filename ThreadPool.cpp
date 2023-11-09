#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <functional>
#include <iostream>
#include <string>
#include <future>
#include <functional>

#include "ThreadSafeQueue.hpp"

class ThreadPool {
    using task_type = MovableNoCopyableFunctionWrapper;

    std::vector<std::thread> threads;
    ThreadSafeQueue<task_type> work_queue;
    std::atomic_bool done;

    void worker_thread() {
        std::cout << "worker thread \n" ;

        task_type task;
        while (!done) {
            if (work_queue.try_pop(task)) // will call move assignment
            {
                std::cout << "try pop " << std::endl;
                task();
            }
            else {
                std::this_thread::yield();
            }
        }
    }
    public:
    ThreadPool(const unsigned int& num_worker_threads): 
            done(false), 
            threads () 
    {   
        unsigned int n_threads = std::thread::hardware_concurrency() - 1;
        
        n_threads = num_worker_threads > 0 ? num_worker_threads : 0;
        n_threads = num_worker_threads > std::thread::hardware_concurrency() - 1 ? std::thread::hardware_concurrency() - 1 : num_worker_threads;

        try {
            for (unsigned int i = 0; i < n_threads; ++i) {
                threads.push_back(std::thread(&ThreadPool::worker_thread, this));
            }
        }
        catch (...) {
            std::cout << "some error" << std::endl;
            done = true;
            throw;
        }
    
    }


    template <  typename FunctionType, typename ResultType = typename std::result_of<FunctionType()>::type>
    std::future<ResultType> submit(FunctionType f) {
        std::packaged_task<ResultType()> task(std::move(f));
        std::future<ResultType> res (task.get_future());
        work_queue.push(std::move(task));
        return res;
    }


    ~ThreadPool() {
        std::cout << "Destroy thread pool\n";
        done = true;
        for (unsigned int i = 0; i < threads.size(); ++i) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }
    }

};



void f() {
    std::cout << "Hello world from thread pool\n";
}

int main() {
    ThreadPool thread_pool(5); // A thead pool with 5 thread

    std::list<int> intList {1,2,3};
    thread_pool.submit(f);

    std::packaged_task<int()> task(std::bind(accumulation_block<int>(), intList.begin(), intList.end()));
    std::future<int> the_future = task.get_future();
    thread_pool.submit(std::move(task));

    std::cout << "accumulation value: " << the_future.get() << std::endl;

}
