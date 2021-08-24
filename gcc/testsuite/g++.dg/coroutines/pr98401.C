//  { dg-do run }

#include <iostream>
#include <coroutine>

struct lifetime_tester {
  lifetime_tester* const ptr;
  lifetime_tester():ptr(this) {
    std::cout << "Constructed at " << this << "\n";
  }
  lifetime_tester(lifetime_tester const& other):ptr(this) {
    std::cout << "Copy constructed from " << &other << " to " << this << "\n";
  }
  lifetime_tester(lifetime_tester&& other):ptr(this) {
    std::cout << "Move constructed from " << &other << " to " << this << "\n";
  }
  lifetime_tester& operator=(lifetime_tester const& other) {
    std::cout << "Copy assigned from " << &other << " to " << this << "\n";
    return *this;
  }
  lifetime_tester& operator=(lifetime_tester&& other) {
    std::cout << "Move assigned from " << &other << " to " << this << "\n";
    return *this;
  }
  ~lifetime_tester() {
    std::cout << "Destructed at " << this << "\n";
  }
  void output_position() const {
    std::cout << "Poked at " << this << " with ptr to " << ptr << "\n";
  }
};
struct tag {
  lifetime_tester tester;
};
struct routine{
    struct promise_type{
        routine get_return_object(){
            return {};
        }
        auto initial_suspend(){
            return std::suspend_never();
        }
        auto final_suspend() noexcept {
            return std::suspend_never();
        }
        void return_void(){}
        void unhandled_exception(){
          std::terminate();
        }
        auto await_transform(tag&& t) {
          t.tester.output_position();
          return std::suspend_never();
        }
    };
};
routine example() {
  co_await tag{};
  std::cout << "End of coroutine body.\n";
}
int main() {
  example();
}
