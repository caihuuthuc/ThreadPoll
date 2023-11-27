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

#include "ThreadSafeQueue.hpp"
#include "StealingWorkQueue.hpp"

#ifndef  __FUNCTION_WRAPPER_
#define __FUNCTION_WRAPPER_
#include "MoveOnlyFunctionWrapper.hpp"
#endif

class ThreadPool {
    using task_type = move_only_function_wrapper; 
    using local_queue_type = StealingWorkQueue;
    
    unsigned int n_threads;
    std::vector<std::thread> threads;

    std::vector<std::unique_ptr<local_queue_type>> local_queues;
    thread_safe_queue<task_type> work_queue;
    std::atomic_bool done;

    inline static thread_local local_queue_type * local_work_queue;
    inline static thread_local size_t thread_index;

    void worker_thread(size_t index) {
        thread_index = index;
        local_work_queue = local_queues[thread_index].get();
        while (!done) {
            run_pending_task();
        }
    }

    bool steal_work_from_other_threads(task_type& task) {
        auto local_queue_size = local_queues.size();
        for (size_t index = 0; index < local_queue_size - 1; ++index) {
            auto other_thread_index = (thread_index + index + 1) % local_queue_size;
            if (local_queues[other_thread_index]->try_steal(task)) {
                return true;
            }
        }
        return false;
    }
    public:
    ThreadPool(unsigned num_worker_threads): 
            done(false), 
            threads (),
            local_queues (),
            n_threads(std::move(num_worker_threads))
    {   
        try {
            for (unsigned i = 0; i < n_threads; ++i) {
                threads.push_back(std::thread(&ThreadPool::worker_thread, this, static_cast<size_t>(i)));
                local_queues.push_back(std::make_unique<local_queue_type>());
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
            local_work_queue->push(std::move(task));
        }
        else {
            work_queue.push(std::move(task));
        }
        return res;
    }


    void run_pending_task() {
        task_type task;
        if (local_work_queue && local_work_queue->try_pop(task)) {
            task();
        }
        else if (steal_work_from_other_threads(task)) {
            task();
        }
        else if (work_queue.try_pop(task)) // will call move assignment
        {
            task();
        }
        else {
            std::this_thread::yield();
        }
    }

    ~ThreadPool() {
        done = true;
        for (unsigned i = 0; i < threads.size(); ++i) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }
    }
};
