#include <chrono>
#include <iostream>
#include <new>
#include <utility>

using default_clock = std::chrono::system_clock;

#if __has_include(<coroutine>)
#include <coroutine>
namespace coro_ns = std;
#else
#include <experimental/coroutine>
namespace coro_ns = std::experimental;
#endif

void error(const char*);
void debug_print(const char* msg);
void debug_print(int val);

template <typename Clock>
class polled_task_promise;

template <typename Clock = default_clock>
class polled_task;

template <typename Clock>
class polled_task_promise {
  template <typename Promise = void>
  using handle_t = coro_ns::coroutine_handle<Promise>;
public:
  inline static int allocated_frames_count{};
  static void* operator new(std::size_t count) {
    debug_print("allocated frames, total count:");
    debug_print(++allocated_frames_count);
    return ::operator new(count);
  }

  static void operator delete(void* ptr) {
    debug_print("deleting frame, remaining count:");
    debug_print(--allocated_frames_count);
    return ::operator delete(ptr);
  }

  template <typename Rep, typename Period>
  polled_task<Clock> await_transform(
      const std::chrono::duration<Rep, Period> duration);

  template <typename Awaiter>
  Awaiter await_transform(Awaiter&& awaiter) {
    return {std::forward<Awaiter>(awaiter)};
  }

  coro_ns::suspend_always initial_suspend() noexcept { return {}; }

  coro_ns::suspend_always final_suspend() noexcept { return {}; }

  void unhandled_exception() noexcept { error("unhandled"); }

  void return_void() noexcept {}

  polled_task<Clock> get_return_object() noexcept;
};

template <typename Clock>
class [[nodiscard]] polled_task {
 public:
  using promise_type = polled_task_promise<Clock>;
  using handle_type = coro_ns::coroutine_handle<promise_type>;

 public:
  polled_task() noexcept = default;

  explicit polled_task(handle_type coroutine) : m_coroutine(coroutine) {}

  polled_task(polled_task&& t) noexcept : m_coroutine(t.m_coroutine) {
    t.m_coroutine = nullptr;
  }

  /// Disable copy construction/assignment.
  polled_task(const polled_task&) = delete;
  polled_task& operator=(const polled_task&) = delete;

  /// Frees resources used by this task.
  ~polled_task() {
    if (m_coroutine) {
      m_coroutine.destroy();
    }
  }

  polled_task& operator=(polled_task&& other) noexcept {
    if (&other != this) {
      if (m_coroutine) {
        m_coroutine.destroy();
      }

      m_coroutine = other.m_coroutine;
      other.m_coroutine = nullptr;
    }

    return *this;
  }

  bool operator()() {
    m_coroutine.resume();
    return m_coroutine.done();
  }

  /// \brief
  /// Query if the task result is complete.
  ///
  /// Awaiting a task that is ready is guaranteed not to block/suspend.
  bool is_ready() const noexcept { return !m_coroutine || m_coroutine.done(); }

  auto operator co_await() noexcept {
    struct awaitable {
      handle_type m_coroutine;

      awaitable(handle_type coroutine) noexcept : m_coroutine(coroutine) {}

      bool await_ready() const noexcept { return m_coroutine.done(); }

      bool await_suspend(handle_type) noexcept { return false; }

      void await_resume() {}
    };

    return awaitable{m_coroutine};
  }

 private:
  coro_ns::coroutine_handle<promise_type> m_coroutine{};
};

template <typename Clock>
polled_task<Clock> polled_task_promise<Clock>::get_return_object() noexcept {
  return polled_task<Clock>{
      coro_ns::coroutine_handle<polled_task_promise<Clock>>::from_promise(*this)};
}

template <typename Clock, typename Rep, typename Period>
auto wait(std::chrono::duration<Rep, Period> duration)
-> polled_task<Clock> {
  using namespace std::chrono;
  const auto begin = Clock::now();
  while (begin + duration > Clock::now()) {
    co_await coro_ns::suspend_always{};
  }
}

template <typename Clock>
template <typename Rep, typename Period>
auto polled_task_promise<Clock>::await_transform(
    const std::chrono::duration<Rep, Period> duration)
-> polled_task<Clock>
{
  co_await wait<Clock>(duration);
}

constexpr bool debug_mode = true;

void debug_print(const char* message) {
  if constexpr (debug_mode) {
    std::cout << message << "\n";
  }
}

void debug_print(int val) {
  if constexpr (debug_mode) {
    std::cout << std::dec << val << "\n";
  }
}

void error(const char* message) {
  debug_print(message ? message : "error");
  exit(1);
}

polled_task<> coro() {
  co_await std::chrono::seconds{2};
}

int main() {
  using namespace std::chrono_literals;

  auto val_task = coro();
  while (not val_task()) {
  }

  std::cout << "hello coro-world" << "\n";
}
