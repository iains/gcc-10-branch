//  { dg-do run }

#include <coroutine>
#include <exception>
#include <utility>
#include <cassert>
#include <cstdio>

struct task {
  struct promise_type {
        task get_return_object() noexcept {
            std::puts("task::promise_type get_return_object");
            return task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept {
            std::puts("task::promise_type initial_suspend");
            return {};
        }
        
        struct yield_awaiter {
            bool await_ready() noexcept { return false; }
            auto await_suspend(std::coroutine_handle<promise_type> h) noexcept {
               std::printf("task::yield_awaiter yielding/returning value h %p cont %p\n",
                            h.address(),  h.promise().continuation.address());
                if (h.promise().continuation.address())
                  return std::exchange(h.promise().continuation, {});
                return std::coroutine_handle<>(std::noop_coroutine());
            }
            void await_resume() noexcept {
                std::puts("task::yield_awaiter::res resumed");
            }
        };

        yield_awaiter yield_value(int x) noexcept {
            std::printf("task::promise_type yield_value %d\n", x);
            value = x;
            return {};
        }

        void return_value(int x) noexcept {
            std::printf("task::promise_type return_value %d\n", x);
            value = x;
        }

        yield_awaiter final_suspend() noexcept {
            std::printf("task::promise_type final_suspend\n");
            return {};
        }

        [[noreturn]] void unhandled_exception() noexcept {
            std::terminate();
        }

        int value;
        std::coroutine_handle<> continuation;
    };

    explicit task(std::coroutine_handle<promise_type> coro) noexcept
    : coro(coro)
    { std::printf("task(coro) %p\n", coro.address()); }

    task(task&& t) noexcept
    : coro(std::exchange(t.coro, {}))
    { std::printf("task(task&& t) %p\n", coro.address()); }

    ~task() {
        std::printf("~task() %p\n", coro ? coro.address() : 0);
        if (coro) coro.destroy();
    }

    __attribute__((noinline)) bool await_ready() {
        std::puts("task::await_ready");
        return false;
    }

    __attribute__((noinline)) auto await_suspend(std::coroutine_handle<> h) noexcept {
        std::printf("task::await_suspend h %p\n", h.address());
        coro.promise().continuation = h;
        return coro;
    }

    __attribute__((noinline)) int await_resume() {
        std::puts("task::await_resume");
        return coro.promise().value;
    }

    std::coroutine_handle<promise_type> coro;
};

task two_values() {
    std::puts("two_values");
    co_yield 1;
    co_return 2;
}

task example() {
    std::puts("example");
    task t = two_values();
    int result = co_await t + co_await t;
    //result = result + co_await t;
    std::printf("result = %i (should be 3)\n", result);
    std::fflush(stdout);
    co_return result;
}

int main() {
    std::printf("creating\n");
    task t = example();
    if (!t.coro.done()) {
      std::printf("was not done, resuming\n");
      t.coro.resume();
    }
    std::printf("should be done\n");
    //std::printf("about to check done\n");
    //assert(t.coro.done());
}
