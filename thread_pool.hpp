#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <memory>
#include <functional>
#include <iostream>
#include <string>
#include <future>
#include <functional>

#include "thread_safe_queue.hpp"
#include "movable_no_copyable_function_wrapper.hpp"

class thread_pool {
    using task_type = movable_no_copyable_function_wrapper; 
    using local_queue_type = std::deque<task_type>;
    
    unsigned int n_threads;
    std::vector<std::thread> threads;

    std::vector<local_queue_type> local_queues;
    ThreadSafeQueue<task_type> work_queue;
    std::atomic_bool done;

    inline static thread_local local_queue_type * local_work_queue;
    inline static thread_local size_t thread_index;

    void worker_thread(size_t index) {
        thread_index = index;
        local_work_queue = &local_queues[thread_index];
        while (!done) {
            run_pending_task();
        }
    }


    void run_pending_task() {
        task_type task;
        if (local_work_queue && !local_work_queue->empty()) {
            task = std::move(local_work_queue->front());
            local_work_queue->pop_front();
            task();
        }
        else if (work_queue.try_pop(task))
        {
            task();
        }
        else {
            std::this_thread::yield();
        }
    }

    public:
    thread_pool(unsigned num_worker_threads): 
            done(false), 
            threads (),
            local_queues (),
            n_threads(std::move(num_worker_threads))
    {   
        try {
            local_queues.resize(n_threads);
            for (unsigned i = 0; i < n_threads; ++i) {
                threads.push_back(std::thread(&thread_pool::worker_thread, this, static_cast<size_t>(i)));
            }
        }
        catch (...) {
            std::cout << "some errors" << std::endl;
            done = true;
            throw;
        }
    }


    template <typename FunctionType, typename ... Args, typename ResultType = typename std::result_of<FunctionType(Args...)>::type>
    std::future<ResultType> submit(FunctionType&& f, Args&& ... args) {
        std::packaged_task<ResultType()> task(
            std::bind(std::forward<FunctionType>(f), std::forward<Args>(args)...)
        );
        std::future<ResultType> res (task.get_future());

        if (local_work_queue) {
            local_work_queue->push_back(std::move(task));
        }
        else {
            work_queue.push(std::move(task));
        }
        return res;
    }


    ~thread_pool() {
        done = true;
        for (unsigned i = 0; i < threads.size(); ++i) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }
    }
};
