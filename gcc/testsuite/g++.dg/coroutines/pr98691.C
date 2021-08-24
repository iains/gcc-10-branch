// { dg-do run }
#include <iostream>
#include <utility>
#include <cstdio>

#ifdef __clang__
#include <experimental/coroutine>
namespace std {
  using namespace std::experimental;
}
#else
#include <coroutine>
#endif

class promise;

class task {
 public:
  using promise_type = promise;
  using handle_type = std::coroutine_handle<promise_type>;

  explicit task(handle_type handle) : handle_(handle)
   {/*std::printf("task(handle_type handle) %p\n", handle_.address());*/}
  ~task() { /*std::cout << "~task()" << std::endl;*/}
  
  task(task&& other) : handle_(std::exchange(other.handle_, nullptr))
   { /*std::cout << "task&& other" << std::endl;*/}

  task& operator=(task&& other) {
    /*std::cout << "task& operator=" << std::endl;*/
    if (handle_) handle_.destroy();
    handle_ = std::exchange(other.handle_, nullptr);
    return *this;
  }

 private:
  handle_type handle_;

  task(const task&) = delete;
  task& operator=(const task&) = delete;
};

class promise {
 public:
  promise() = default;
  ~promise() {/*std::cout << "~promise()" << std::endl;*/}

  void* operator new(size_t sz) {
    /*std::puts("operator new()");*/
    return ::operator new(sz);
  }

  void operator delete(void* p, size_t sz) {
    /*std::puts("operator delete");*/
    return ::operator delete(p, sz);
  }

  std::suspend_never initial_suspend() noexcept { return {}; }
  std::suspend_never final_suspend() noexcept { return {}; }
  void unhandled_exception() noexcept { abort(); }

  task get_return_object() {
    return task{std::coroutine_handle<promise>::from_promise(*this)};
  }

  void return_value(int rv) {
    std::cout << rv << std::endl;
  }

 private:
  promise(const promise&) = delete;
  promise operator=(const promise&) = delete;
};

struct suspend_never_with_int {
  bool await_ready() { return true; }
  void await_suspend(std::coroutine_handle<>) {}
  int await_resume() { return 42; }
};

suspend_never_with_int foo() {
  std::cout << "NOTREACHED" << std::endl;
  abort ();
  return {};
}

__attribute__((__noinline__))
bool abool (int y)
{
  return y != 10;
}

task failcase() {
  bool t = abool (10);
  int val = t ? co_await foo() : 1;
  co_return val; 
}

int main() {
  failcase();
  return 0;
}
