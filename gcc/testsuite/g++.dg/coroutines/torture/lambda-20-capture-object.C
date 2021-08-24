//  { dg-do compile { target c++17 } }
//  { dg-additional-options "-Wno-return-type" }

#include "../coro.h"
#include <tuple>
#include <functional>

template <typename, typename = std::void_t<>> struct a;
template <typename b>
struct a<b, std::void_t<decltype(std::declval<b>().await_resume())>>
    : std::conjunction<> {};
template <typename b>
auto c(b value, int) -> decltype(value.operator co_await());
template <typename b, std::enable_if_t<a<b>::value, int> = 0> b c(b, int);
template <typename b> auto d(b value) -> decltype(c(value, 3)) {}
template <typename b> struct f {
  using e = decltype(d(std::declval<b>()));
  using h = decltype(std::declval<e>().await_resume());
};
template <typename> class s;
template <typename... g> class s<std::tuple<g...>> {
public:
  s(std::tuple<g...>);
  auto operator co_await() {
    struct aa {
      std::tuple<g...> await_resume() {};
      s i;
    };
    return aa{*this};
  }
};
template <typename j> class k {
public:
  using l = std::coroutine_handle<k>;
  j m();
};
template <typename j> class ab {
public:
  using promise_type = k<j>;
  using l = typename promise_type::l;
  auto m() { return n.promise().m(); }
  auto ac() { return m(); }
  l n;
};
template <typename o, typename j = typename f<o>::h,
          std::enable_if_t<!std::is_void_v<j>, int> = 0>
ab<j> p(o);
template <typename b> struct u { using ad = b; };
template <typename b> using t = typename u<b>::ad;
template <typename... ae, std::enable_if_t<std::conjunction_v<>, int> = 0>
auto af(ae... ag) {
  return s<std::tuple<ab<typename f<t<std::remove_reference_t<ae>>>::h>...>>(
      std::make_tuple(p(ag)...));
}
template <typename q, typename o> class ah {
  using e = typename f<o>::e;

public:
  ah(q ai, o aj) : r(ai), ak(d(aj)) {}
  auto await_ready() { return 0; }
  template <typename v> auto await_suspend(std::coroutine_handle<v>) {}
  template <typename w = decltype(0),
            std::enable_if_t<!std::is_void_v<w>, int> = 0>
  auto await_resume() {
    return invoke(r, ak.await_resume());
  }
  q r;
  e ak;
};
template <typename q, typename o> class x {
public:
  template <typename y, typename al,
            std::enable_if_t<std::is_constructible_v<o, al>, int> = 0>
  x(y ai, al aj) : r(ai), i(aj) {}
  auto operator co_await() { return ah(r, i); }
  q r;
  o i;
};
template <typename q, typename o> auto am(q ai, o aj) {
  return x<std::remove_cv_t<std::remove_reference_t<q>>,
           std::remove_cv_t<std::remove_reference_t<o>>>(ai, aj);
}
template <typename... ae, std::enable_if_t<std::conjunction_v<>, int> = 0>
auto an(ae... ag) {
  return am(
      [](auto ao) {
        auto ap =
            apply([](auto... aq) { return std::make_tuple(aq.ac()...); }, ao);
        return ap;
      },
      af(ag...));
}
class ar;
class z {
public:
  ar as();
};
class at {
public:
  ~at();
};
class ar {
public:
  at await_resume();
};
class au;
class av {
  struct aw {
    bool await_ready() noexcept;
    template <typename v> void await_suspend(std::coroutine_handle<v>) noexcept;
    void await_resume() noexcept;
  };

public:
  auto initial_suspend() { return std::suspend_always{}; }
  auto final_suspend() noexcept { return aw{}; }
};
class ax : public av {
public:
  au get_return_object();
  void unhandled_exception();
};
class au {
public:
  using promise_type = ax;
};
void d() {
  []() -> au {
    z ay;
    co_await an(ay.as());
  };
}
