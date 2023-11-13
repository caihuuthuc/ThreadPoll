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


class ThreadPool {
    using task_type = MovableNoCopyableFunctionWrapper;
    using local_queue_type = std::deque<task_type>;
    
    unsigned int n_threads;
    std::vector<std::thread> threads;

    std::vector<local_queue_type> local_queues;
    ThreadSafeQueue<task_type> work_queue;
    std::atomic_bool done;

    inline static thread_local local_queue_type * local_work_queue;
    inline static thread_local size_t thread_index;

    unsigned int get_n_threads(const unsigned int & n_threads_) {
        unsigned int res = std::thread::hardware_concurrency() - 1;
        
        res = n_threads_ > 0 ? n_threads_ : 0;
        res = res > std::thread::hardware_concurrency() - 1 ? std::thread::hardware_concurrency() - 1 : res;
        return res;
    }


    void worker_thread(size_t index) {
        // local_work_queue = local_work_queues[worker_index].get();
        // index = 1;
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
        }
        else if (work_queue.try_pop(task)) // will call move assignment
        {

            task();
        }
        else {
            std::this_thread::yield();
        }

    }

    public:
    ThreadPool(const unsigned int& num_worker_threads): 
            done(false), 
            threads (),
            local_queues (),
            n_threads(get_n_threads(num_worker_threads))
    {   
        try {
            local_queues.resize(n_threads);
            for (unsigned int i = 0; i < n_threads; ++i) {
                threads.push_back(std::thread(&ThreadPool::worker_thread, this, static_cast<size_t>(i)));
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


    ~ThreadPool() {
        done = true;
        for (unsigned int i = 0; i < threads.size(); ++i) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }
    }
};
