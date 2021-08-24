// { dg-do run  { target c++17 } }

#if __has_include(<coroutine>)
#include <coroutine>
#else
#include <experimental/coroutine>
namespace std { using namespace experimental; }
#endif

#include <tuple>
#include <iostream>
#include <optional>

struct retry {
    int max_retries = 10;
    std::coroutine_handle<> *handle = nullptr;
};

template<typename R, typename Callable, typename ...T>
struct std::coroutine_traits<R, Callable, retry, T...> {
    struct promise_type {
        Callable c;
        std::tuple<retry, T...> args;

        promise_type(Callable c, retry r, T ...args) : c(c), args(r, args...) {}

        // FIXME: Also handle non-void return.
        void get_return_object() {}
        void return_void() {}

        void unhandled_exception() { throw; }

        auto initial_suspend() {
            struct awaitable {
                bool await_ready() { return !r.handle; }
                void await_suspend(std::coroutine_handle<> h) { *r.handle = h; }
                void await_resume() {}
                retry r;
            };
            return awaitable{get<0>(args)};
        }
        std::suspend_never final_suspend() noexcept { return {}; }

        auto await_transform(retry r) {
            if (--get<0>(args).max_retries == 0)
                throw "out of retries";
            struct awaitable {
                bool await_ready() { return false; }
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) {
                    promise_type &p = h.promise();
                    std::coroutine_handle<> new_h;
                    get<0>(p.args).handle = &new_h;
                    std::apply(p.c, p.args);
                    h.destroy();
                    return new_h;
                }
                [[noreturn]] void await_resume() { __builtin_unreachable(); }
            };
            return awaitable{};
        }
    };
};

struct Lambda {
    int n = 0;
    void operator()(retry r, int a, int b, int c) {
        //std::cout << "trying " << a << " " << b << " " << c << " ... ";
        a += b;
        if (++n != 3) {
         //   std::cout << "failed\n";
            co_await r;
        }
        //std::cout << "OK!\n";
        co_return;
    }
};

int main() {
    Lambda l;
    l(retry{5}, 1, 2, 3);
}
