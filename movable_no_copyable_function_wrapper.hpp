#include <memory>

class move_only_function_wrapper {
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
        move_only_function_wrapper() = default;

        template <typename F>
        move_only_function_wrapper(F&& f): impl(new impl_type<F>(std::move(f))) 
        {}

        move_only_function_wrapper(move_only_function_wrapper&& other) : 
                impl(std::move(other.impl)) 
        {}

        move_only_function_wrapper& operator=(move_only_function_wrapper&& other) {
            impl = std::move(other.impl);
            return *this;
        }

        void operator() () {impl->call();}

        move_only_function_wrapper(const move_only_function_wrapper&) = delete;
        move_only_function_wrapper& operator=(const move_only_function_wrapper&) = delete;
};
