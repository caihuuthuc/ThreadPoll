#include <memory>

class movable_no_copyable_function_wrapper {
    struct impl_base {
        virtual void call() = 0;
        virtual ~impl_base() {};
    };

    std::unique_ptr<impl_base> impl;

    template <typename F>
    struct impl_type: impl_base {
        F f;
        impl_type(F&& f_): f(std::move(f_)) {}
        void call() { f(); }
    };

    public:
        movable_no_copyable_function_wrapper() = default;

        template <typename F>
        movable_no_copyable_function_wrapper(F&& f): impl(new impl_type<F>(std::move(f))) 
        {}

        movable_no_copyable_function_wrapper(movable_no_copyable_function_wrapper&& other) : 
                impl(std::move(other.impl)) 
        {}

        movable_no_copyable_function_wrapper& operator=(movable_no_copyable_function_wrapper&& other) {
            impl = std::move(other.impl);
            return *this;
        }

        void operator() () {impl->call();}

        movable_no_copyable_function_wrapper(const movable_no_copyable_function_wrapper&) = delete;
        movable_no_copyable_function_wrapper& operator=(const movable_no_copyable_function_wrapper&) = delete;
};
