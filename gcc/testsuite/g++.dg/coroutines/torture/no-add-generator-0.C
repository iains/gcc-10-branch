//  { dg-do run }

#include "../coro.h"

#include <numeric>
 
template <typename T> 
struct generator {

  struct promise_type {
    T current_value;
    coro::suspend_always yield_value(T value) {
      this->current_value = value;
      return {};
    }
    coro::suspend_always initial_suspend() { return {}; }
    coro::suspend_always final_suspend() noexcept { return {}; }

    generator get_return_object() { return generator{this}; };

    void unhandled_exception() { /*std::terminate();*/  }
    void return_void() {}
  };

  struct iterator {
    coro::coroutine_handle<promise_type> _Coro;
    bool _Done;

    iterator(coro::coroutine_handle<promise_type> Coro, bool Done)
        : _Coro(Coro), _Done(Done) {}

    iterator &operator++() {
      _Coro.resume();
      _Done = _Coro.done();
      return *this;
    }

    bool operator==(iterator const &_Right) const {
      return _Done == _Right._Done;
    }

    bool operator!=(iterator const &_Right) const { return !(*this == _Right); }
    T const &operator*() const { return _Coro.promise().current_value; }
    T const *operator->() const { return &(operator*()); }
  };

  iterator begin() {
    p.resume();
    return {p, p.done()};
  }

  iterator end() { return {p, true}; }

  generator() = default;
  generator(generator const&) = delete;
  generator(generator &&rhs) : p(rhs.p) { rhs.p = nullptr; }

  ~generator() {
    if (p)
      p.destroy();
  }

private:
  explicit generator(promise_type *p)
      : p(coro::coroutine_handle<promise_type>::from_promise(*p)) {}

  coro::coroutine_handle<promise_type> p;
};

template <typename T>
generator<T> sequence_gen () noexcept {
  for (T i = {};; ++i)
    co_yield i;
}

template <typename T>
generator<T>
take_until_limit (generator<T>& g, T limit) noexcept {
  for (T v: g)
    if (v < limit) co_yield v;
    else break;
}

template <typename T>
generator<T> multiply_by_factor (generator<T>& g, T factor) noexcept {
  for (auto&& v: g)
    co_yield v * factor;
}

template <typename T>
generator<T> add_constant (generator<T>& g, T constant) noexcept {
  for (auto&& v: g)
    co_yield v + constant;
}

int main() {
  auto s = sequence_gen<int> ();
  auto t = take_until_limit (s, 10);
  auto m = multiply_by_factor (t, 13);
  auto a = add_constant (m, 7);
  unsigned c = 0;
  int ans = 0;
  for (auto X : a)
    {
    PRINTF (" value %d is %d\n", c++, X);
    ans += X;
    }
  //int ans = std::accumulate(a.begin(), a.end(), 0);
  PRINTF (" and the answer is ... %d\n", ans);
  if (ans != 655)
   abort();
  return 0;
}