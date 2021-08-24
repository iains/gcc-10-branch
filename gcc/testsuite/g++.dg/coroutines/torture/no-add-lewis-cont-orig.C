//  { dg-do run }
// Lewis' test of continuation.

#if __has_include (<coroutine>)
#include <coroutine>
#include <exception>
using namespace std;
#elif __has_include (<experimental/coroutine>)
#include <experimental/coroutine>
#include <exception>
using namespace std::experimental;
#endif

#define ENABLE_LOGGING 1


#if ENABLE_LOGGING
#include <stdio.h>

inline void log(const char* s) {
    printf("%s\n", s);
    fflush(stdout);
}

#else

inline void log(const char* s) {}

#endif


struct task {
    struct promise_type {
        coroutine_handle<> continuation_;

        task get_return_object() noexcept {
            return task{coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_always initial_suspend() noexcept { return{}; }
        
        auto final_suspend() noexcept {
            struct awaiter {
                bool await_ready() noexcept {
                    return false;
                }
                coroutine_handle<> await_suspend(coroutine_handle<promise_type> h) noexcept {
                    return h.promise().continuation_;
                }
                void await_resume() noexcept {}
            };
            return awaiter{};
        }

        void return_void() noexcept {}

        void unhandled_exception() noexcept { std::terminate(); }
    };

    coroutine_handle<promise_type> coro_;

    explicit task(coroutine_handle<promise_type> h) noexcept : coro_(h) {}

    task(task&& t) noexcept : coro_(t.coro_) {
        t.coro_ = {};
    }

    ~task() {
        if (coro_)
            coro_.destroy();
    }

    auto operator co_await() && noexcept {
        struct awaiter {
            coroutine_handle<promise_type> coro_;
            bool await_ready() noexcept {
                return false;
            }
            auto await_suspend(coroutine_handle<> continuation) noexcept {
                coro_.promise().continuation_ = continuation;
                return coro_;
            }
            void await_resume() noexcept {}
        };
        return awaiter{coro_};
    }
};

struct sync_wait_task {
    struct promise_type {
        sync_wait_task get_return_object() noexcept {
            return sync_wait_task{coroutine_handle<promise_type>::from_promise(*this)};
        }

        suspend_never initial_suspend() noexcept { return{}; }
        
        suspend_always final_suspend() noexcept { return{}; }

        void return_void() noexcept {}

        void unhandled_exception() noexcept { std::terminate(); }
    };

    coroutine_handle<promise_type> coro_;

    explicit sync_wait_task(coroutine_handle<promise_type> h) noexcept : coro_(h) {}

    sync_wait_task(sync_wait_task&& t) noexcept : coro_(t.coro_) {
        t.coro_ = {};
    }

    ~sync_wait_task() {
        if (coro_) {
            coro_.destroy();
        }
    }

    static sync_wait_task start(task t) {
        co_await std::move(t);
    }

    bool done() {
        return coro_.done();
    }
};

struct manual_executor {
    struct schedule_op {
        manual_executor& executor_;
        schedule_op* next_ = nullptr;
        coroutine_handle<> continuation_;

        schedule_op(manual_executor& executor)
        : executor_(executor)
        {}

        bool await_ready() noexcept { return false; }

        void await_suspend(coroutine_handle<> continuation) noexcept {
            continuation_ = continuation;
            next_ = executor_.head_;
            executor_.head_ = this;
        }

        void await_resume() noexcept {}
    };

    schedule_op* head_ = nullptr;

    schedule_op schedule() noexcept {
        return schedule_op{*this};
    }

    void drain() {
        while (head_ != nullptr) {
            log("processing item");
            auto* item = head_;
            head_ = item->next_;
            item->continuation_.resume();
        }
        log("drain() returning");
    }

    void sync_wait(task&& t) {
        auto t2 = sync_wait_task::start(std::move(t));
        while (!t2.done()) {
            drain();
        }
        log("sync_wait() returning");
    }
};

task completes_synchronously() {
    co_return;
}

task synchronous_loop() {
    log("synchronous_loop()");

    // Test that we can complete synchronously in a loop 1M times
    // without overflowing the stack.
    for (int i = 0; i < 1'000'000; ++i) {
        co_await completes_synchronously();
    }

    log("synchronous_loop() returning");
}

task foo(manual_executor& ex) {
    log("foo() scheduling");

    co_await ex.schedule();

    log("foo() completing");

    co_return;
}

task bar(manual_executor& ex) {
    log("bar() started");

    co_await foo(ex);

    log("bar() completing");
}

int main() {
    manual_executor ex;

    log("bar() starting");
    ex.sync_wait(bar(ex));
    log("bar() completed");

    ex.sync_wait(synchronous_loop());

    return 0;
}
