#include <coroutine>
#include <initializer_list>

struct a {
  a();
};

struct task {
  struct promise_type {
    std::suspend_always initial_suspend();
    std::suspend_always final_suspend() noexcept;
    void unhandled_exception();
    task get_return_object();
  };

  std::suspend_always c(a);

  a bar(std::initializer_list<a>);

 task e() {
    co_await c(bar({a{}}));
  }
};
