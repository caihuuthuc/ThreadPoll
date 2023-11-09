#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>

template <typename T>
class ThreadSafeQueue {
    struct Node {
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
    };
    
    std::unique_ptr<Node> head;
    Node* tail;
    mutable std::mutex head_mutex;
    mutable std::mutex tail_mutex;
    mutable std::condition_variable data_cond;

    Node * get_tail() {
        std::lock_guard<std::mutex> l(tail_mutex);
        return tail;
    }

    T pop_head() {
        std::unique_ptr<Node> const old_head = std::move(head);
        head=std::move(old_head->next);
        return old_head;

    }
    std::unique_lock<std::mutex> wait_for_data() {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock, [&] { return head.get() != get_tail(); });
        return head_lock;
    }

    std::unique_ptr<Node> wait_pop_head(T& res) {
        std::unique_lock<std::mutex> l(wait_for_data());

        res = std::move(*head->data);
        return pop_head();
    }

    std::unique_ptr<Node> wait_pop_head() {
        std::unique_lock<std::mutex> l(wait_for_data());
        return pop_head();
    }

    public:
    ThreadSafeQueue(): head(new Node) {tail = head.get();}
    ThreadSafeQueue(ThreadSafeQueue&&) = default;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = default;

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    bool try_pop(T & value) {
        std::lock_guard<std::mutex> l(head_mutex);
        if (head.get() == get_tail()) {
            return false;
        }
        value = *head->data;
        std::unique_ptr<Node> old_head = std::move(head);
        head = std::move(old_head->next);
        return true;
    }

    void push(T new_value) {
        std::shared_ptr<T> new_data( 
                std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<Node> p(new Node);
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data=new_data;
            Node* const new_tail=p.get();
            tail->next=std::move(p);
            tail=new_tail;
        }
        data_cond.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_ptr<Node> const old_head = wait_pop_head(value);
    }

    std::unique_ptr<T> wait_and_pop() {
        std::unique_ptr<Node> const old_head = wait_pop_head();
        return old_head->data;
    }

    void print() const {
        std::lock_guard<std::mutex> lock_tail(tail_mutex); // prevent pushing new data
        std::lock_guard<std::mutex> lock_head(head_mutex); // then prevent pop data
        Node * node = head.get();
        while (node != tail) {
            std::cout << *node->data << " ";
            node = node->next.get();
        }
        std::cout << std::endl;
    }
};
