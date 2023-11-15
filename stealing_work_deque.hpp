#include <queue>
#include <mutex>

#ifndef  __FUNCTION_WRAPPER_
#define __FUNCTION_WRAPPER_
#include "movable_no_copyable_function_wrapper.hpp"
#endif


class stealing_work_deque {
    using task_type = movable_no_copyable_function_wrapper;
    std::deque<task_type> work_deques;
    mutable std::mutex deque_mutex;

    public:
    stealing_work_deque() {};

    void push(task_type task) {
        std::lock_guard l(deque_mutex);
        work_deques.push_front(std::move(task));
    }

    bool try_pop(task_type & task) {
        std::lock_guard l(deque_mutex);
        if (work_deques.empty()) return false;
        task = std::move(work_deques.front());
        work_deques.pop_front();
        return true;
    }

    bool empty() {
        std::lock_guard l(deque_mutex);
        return work_deques.empty();
    }

    bool try_steal(task_type & task) {
        std::lock_guard l(deque_mutex);
        if (work_deques.empty()) return false;
        task = std::move(work_deques.back());
        work_deques.pop_back();
        return true;
    }

};
