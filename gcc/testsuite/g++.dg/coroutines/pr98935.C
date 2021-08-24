//  { dg-additional-options "-Wno-pedantic" }

#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
namespace std {
using namespace std::experimental;
}
#endif

struct future {
  struct promise_type {
    void return_void() {}
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    future get_return_object() { return {}; }
  };
  bool await_ready() { return false; }
  void await_suspend(std::coroutine_handle<>) {}
  void await_resume() {}
};

future failcase() {
  //auto t = ({auto x = future{}; x.await_resume(); x;});
  co_await ({auto x = future{}; x;});
}
