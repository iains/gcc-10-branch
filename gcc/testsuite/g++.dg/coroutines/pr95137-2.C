// { dg-do run }
#include <cassert>
#include <functional>
#include <memory>
#include <utility>
#include <unistd.h>

#ifndef __clang__
#include <coroutine>
#define SEASTAR_INTERNAL_COROUTINE_NAMESPACE std
#else
#define SEASTAR_INTERNAL_COROUTINE_NAMESPACE std::experimental
namespace std::experimental {

template <typename Promise>
class coroutine_handle {
    void* _pointer = nullptr;

public:
    coroutine_handle() = default;

    coroutine_handle& operator=(nullptr_t) noexcept {
        _pointer = nullptr;
        return *this;
    }

    explicit operator bool() const noexcept {
        return _pointer;
    }

    static coroutine_handle from_address(void* ptr) noexcept {
        coroutine_handle hndl;
        hndl._pointer = ptr;
        return hndl;
    }
    void* address() const noexcept {
        return _pointer;
    }

    static coroutine_handle from_promise(Promise& promise) noexcept {
        coroutine_handle hndl;
        hndl._pointer = __builtin_coro_promise(&promise, alignof(Promise), true);
        return hndl;
    }
    Promise& promise() const noexcept {
        return *reinterpret_cast<Promise*>(__builtin_coro_promise(_pointer, alignof(Promise), false));
    }

    void operator()() noexcept {
        resume();
    }

    void resume() const noexcept {
        __builtin_coro_resume(_pointer);
    }
    void destroy() const noexcept {
        __builtin_coro_destroy(_pointer);
    }
    bool done() const noexcept {
        return __builtin_coro_done(_pointer);
    }
};

struct suspend_never {
    constexpr bool await_ready() const noexcept {
        return true;
    }
    template <typename T>
    constexpr void await_suspend(coroutine_handle<T>) noexcept {}
    constexpr void await_resume() noexcept {}
};

struct suspend_always {
    constexpr bool await_ready() const noexcept {
        return false;
    }
    template <typename T>
    constexpr void await_suspend(coroutine_handle<T>) noexcept {}
    constexpr void await_resume() noexcept {}
};

template <typename T, typename... Args>
class coroutine_traits {};

}
#endif

namespace seastar {
template <typename T>
struct function_traits;
template <typename... T>
class future;
class reactor;
class task {
protected:
    ~task() = default;

public:
    explicit task() noexcept {}
    virtual void run_and_dispose() noexcept = 0;
    virtual task* waiting_task() noexcept = 0;
    void make_backtrace() noexcept {}
};
void schedule(task* t) noexcept;
template <class... T>
class promise;
template <class... T>
class promise_base_with_type;
class promise_base;
template <typename T>
struct uninitialized_wrapper_base {
    union any {
        any() noexcept {}
        ~any() {}
        T value;
    } _v;

public:
    uninitialized_wrapper_base() noexcept = default;
    template <typename... U>
    void uninitialized_set(U&&... vs) {
        new (&_v.value) T(std::forward<U>(vs)...);
    }
    T& uninitialized_get() {
        return _v.value;
    }
    const T& uninitialized_get() const {
        return _v.value;
    }
};

template <typename T>
struct uninitialized_wrapper : public uninitialized_wrapper_base<T> {};

struct future_state_base {
    enum class state : uintptr_t {
        invalid = 0,
        future = 1,
        result_unavailable = 2,
        result = 3,
        exception_min = 4,
    };
    union any {
        any() noexcept {
            st = state::future;
        }
        any(state s) noexcept {
            st = s;
        }
        bool valid() const noexcept {
            return st != state::invalid;
        }
        bool available() const noexcept {
            return st == state::result || st >= state::exception_min;
        }
        bool failed() const noexcept {
            return __builtin_expect(st >= state::exception_min, false);
        }
        void check_failure() noexcept {
            if (failed()) {
                assert(0);
            }
        }
        ~any() noexcept {
            check_failure();
        }
        void move_it(any&& x) noexcept {
            if (x.st < state::exception_min) {
                st = x.st;
                x.st = state::invalid;
            } else {
                assert(0);
            }
        }
        any(any&& x) noexcept {
            move_it(std::move(x));
        }
        any& operator=(any&& x) noexcept {
            check_failure();
            move_it(std::move(x));
            return *this;
        }
        bool has_result() const noexcept {
            return st == state::result || st == state::result_unavailable;
        }
        state st;
        void* ex;
    } _u;
    future_state_base() noexcept = default;
    future_state_base(state st) noexcept : _u(st) {}
    future_state_base(future_state_base&& x) noexcept : _u(std::move(x._u)) {}

public:
    ~future_state_base() noexcept = default;

    bool valid() const noexcept {
        return _u.valid();
    }
    bool available() const noexcept {
        return _u.available();
    }
    bool failed() const noexcept {
        return _u.failed();
    }
    void ignore() noexcept;
    future_state_base& operator=(future_state_base&& x) noexcept = default;
    template <typename... U>
    friend struct future_state;
    template <typename... U>
    friend class future;
};
struct ready_future_marker {};
struct future_for_get_promise_marker {};
template <typename... T>
struct future_state : public future_state_base, private uninitialized_wrapper<std::tuple<T...>> {
    future_state() noexcept = default;
    void move_it(future_state&& x) noexcept {
        if (_u.has_result()) {
            this->uninitialized_set(std::move(x.uninitialized_get()));
            x.uninitialized_get().~tuple();
        }
    }
    future_state(future_state&& x) noexcept : future_state_base(std::move(x)) {
        move_it(std::move(x));
    }
    void clear() noexcept {
        if (_u.has_result()) {
            this->uninitialized_get().~tuple();
        }
    }
    ~future_state() noexcept {
        clear();
    }
    future_state& operator=(future_state&& x) noexcept {
        clear();
        future_state_base::operator=(std::move(x));
        move_it(std::move(x));
        return *this;
    }
    template <typename... A>
    future_state(ready_future_marker, A&&... a) noexcept : future_state_base(state::result) {
        this->uninitialized_set(std::forward<A>(a)...);
    }
    template <typename... A>
    void set(A&&... a) {
        assert(_u.st == state::future);
        new (this) future_state(ready_future_marker(), std::forward<A>(a)...);
    }
    std::tuple<T...>&& get_value() && noexcept {
        assert(_u.st == state::result);
        return std::move(this->uninitialized_get());
    }
    std::tuple<T...>&& take_value() && noexcept {
        assert(_u.st == state::result);
        _u.st = state::result_unavailable;
        return std::move(this->uninitialized_get());
    }
    std::tuple<T...>&& take() && {
        assert(_u.st == state::result);
        _u.st = state::result_unavailable;
        return std::move(this->uninitialized_get());
    }
    std::tuple<T...>&& get() && {
        assert(_u.st == state::result);
        return std::move(this->uninitialized_get());
    }
    const std::tuple<T...>& get() const& {
        assert(_u.st == state::result);
        return this->uninitialized_get();
    }
};
template <typename... T>
class continuation_base : public task {
protected:
    future_state<T...> _state;
    using future_type = future<T...>;
    using promise_type = promise<T...>;

public:
    continuation_base() noexcept = default;
    explicit continuation_base(future_state<T...>&& state) noexcept : _state(std::move(state)) {}
    void set_state(future_state<T...>&& state) noexcept {
        _state = std::move(state);
    }
    virtual task* waiting_task() noexcept override {
        return nullptr;
    }
    friend class promise_base_with_type<T...>;
    friend class promise<T...>;
    friend class future<T...>;
};
template <typename Promise, typename... T>
class continuation_base_with_promise : public continuation_base<T...> {
    friend class promise_base_with_type<T...>;

protected:
    continuation_base_with_promise(Promise&& pr, future_state<T...>&& state) noexcept
        : continuation_base<T...>(std::move(state)), _pr(std::move(pr)) {
        task::make_backtrace();
    }
    continuation_base_with_promise(Promise&& pr) noexcept : _pr(std::move(pr)) {
        task::make_backtrace();
    }
    virtual task* waiting_task() noexcept override;
    Promise _pr;
};
template <typename Promise, typename Func, typename... T>
struct continuation final : continuation_base_with_promise<Promise, T...> {
    continuation(Promise&& pr, Func&& func, future_state<T...>&& state)
        : continuation_base_with_promise<Promise, T...>(std::move(pr), std::move(state)), _func(std::move(func)) {}
    continuation(Promise&& pr, Func&& func)
        : continuation_base_with_promise<Promise, T...>(std::move(pr)), _func(std::move(func)) {}
    virtual void run_and_dispose() noexcept override {
        _func(this->_pr, std::move(this->_state));
        delete this;
    }
    Func _func;
};
class future_base;
struct promise_base {
    enum class urgent { no, yes };
    future_base* _future = nullptr;
    future_state_base* _state;
    task* _task = nullptr;
    promise_base(const promise_base&) = delete;
    promise_base(future_state_base* state) noexcept : _state(state) {}
    promise_base(future_base* future, future_state_base* state) noexcept;
    void move_it(promise_base&& x) noexcept;
    promise_base(promise_base&& x) noexcept;
    void clear() noexcept;
    ~promise_base() noexcept {
        clear();
    }
    void operator=(const promise_base&) = delete;
    promise_base& operator=(promise_base&& x) noexcept;
    template <urgent Urgent>
    void make_ready() noexcept;
    friend class future_base;
    template <typename... U>
    friend class seastar::future;

    task* waiting_task() const noexcept {
        return _task;
    }
};
template <typename... T>
struct promise_base_with_type : public promise_base {
    future_state<T...>* get_state() {
        return static_cast<future_state<T...>*>(_state);
    }

    promise_base_with_type(future_state_base* state) noexcept : promise_base(state) {}
    promise_base_with_type(future<T...>* future) noexcept : promise_base(future, &future->_state) {}
    promise_base_with_type(promise_base_with_type&& x) noexcept = default;
    promise_base_with_type(const promise_base_with_type&) = delete;
    promise_base_with_type& operator=(promise_base_with_type&& x) noexcept = default;
    void operator=(const promise_base_with_type&) = delete;
    void set_urgent_state(future_state<T...>&& state) noexcept {
        auto* ptr = get_state();
        if (ptr) {
            assert(ptr->_u.st == future_state_base::state::future);
            new (ptr) future_state<T...>(std::move(state));
            make_ready<urgent::yes>();
        }
    }
    template <typename... A>
    void set_value(A&&... a) {
        if (auto* s = get_state()) {
            s->set(std::forward<A>(a)...);
            make_ready<urgent::no>();
        }
    }
    using promise_base::waiting_task;
    void set_coroutine(future_state<T...>& state, task& coroutine) noexcept {
        _state = &state;
        _task = &coroutine;
    }

    template <typename Pr, typename Func>
    void schedule(Pr&& pr, Func&& func) noexcept {
        auto tws = new continuation<Pr, Func, T...>(std::move(pr), std::move(func));
        _state = &tws->_state;
        _task = tws;
    }
};

template <typename... T>
struct promise : private promise_base_with_type<T...> {
    future_state<T...> _local_state;

    promise() noexcept : promise_base_with_type<T...>(&_local_state) {}
    void move_it(promise&& x) noexcept;
    promise(promise&& x) noexcept : promise_base_with_type<T...>(std::move(x)) {
        move_it(std::move(x));
    }
    promise(const promise&) = delete;
    promise& operator=(promise&& x) noexcept {
        promise_base_with_type<T...>::operator=(std::move(x));
        move_it(std::move(x));
        return *this;
    }
    void operator=(const promise&) = delete;
    using promise_base::waiting_task;
    future<T...> get_future() noexcept;
    template <typename... A>
    void set_value(A&&... a) {
        promise_base_with_type<T...>::set_value(std::forward<A>(a)...);
    }
    using promise_base_with_type<T...>::set_urgent_state;
    template <typename... U>
    friend class future;
};
template <typename T>
struct futurize {
    using type = future<T>;
    using promise_type = promise<T>;
    using value_type = std::tuple<T>;
    template <typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;
    template <typename Func, typename... FuncArgs>
    static inline type invoke(Func&& func, FuncArgs&&... args) noexcept;
    static inline type convert(T&& value) {
        return make_ready_future<T>(std::move(value));
    }
    static inline type convert(type&& value) {
        return std::move(value);
    }
    static type from_tuple(value_type&& value);
    static type from_tuple(const value_type& value);
    template <typename Arg>
    static type make_exception_future(Arg&& arg) noexcept;

    template <typename Func>
    static void satisfy_with_result_of(promise_base_with_type<T>&&, Func&& func);
    template <typename... U>
    friend class future;
};
template <>
struct futurize<void> {
    using type = future<>;
    using promise_type = promise<>;
    using value_type = std::tuple<>;
    template <typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;
    template <typename Func, typename... FuncArgs>
    static inline type invoke(Func&& func, FuncArgs&&... args) noexcept;
    template <typename Func, typename... FuncArgs>
    [[deprecated("Use invoke for varargs")]] static inline type apply(Func&& func, FuncArgs&&... args) noexcept;
    static inline type from_tuple(value_type&& value);
    static inline type from_tuple(const value_type& value);
    template <typename Arg>
    static type make_exception_future(Arg&& arg) noexcept;

    template <typename Func>
    static void satisfy_with_result_of(promise_base_with_type<>&&, Func&& func);
    template <typename... U>
    friend class future;
};
template <typename... Args>
struct futurize<future<Args...>> {
    using type = future<Args...>;
    using promise_type = promise<Args...>;
    using value_type = std::tuple<Args...>;
    template <typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;
    template <typename Func, typename... FuncArgs>
    static inline type invoke(Func&& func, FuncArgs&&... args) noexcept;
    static inline type from_tuple(value_type&& value);
    static inline type from_tuple(const value_type& value);
    static inline type convert(Args&&... values) {
        return make_ready_future<Args...>(std::move(values)...);
    }
    static inline type convert(type&& value) {
        return std::move(value);
    }
    template <typename Arg>
    static type make_exception_future(Arg&& arg) noexcept;

    template <typename Func>
    static void satisfy_with_result_of(promise_base_with_type<Args...>&&, Func&& func);
    template <typename... U>
    friend class future;
};
template <typename T>
using futurize_t = typename futurize<T>::type;
struct future_base {
    promise_base* _promise;
    future_base() noexcept : _promise(nullptr) {}
    future_base(promise_base* promise, future_state_base* state) noexcept : _promise(promise) {
        _promise->_future = this;
        _promise->_state = state;
    }
    void move_it(future_base&& x, future_state_base* state) noexcept {
        _promise = x._promise;
        if (auto* p = _promise) {
            x.detach_promise();
            p->_future = this;
            p->_state = state;
        }
    }
    future_base(future_base&& x, future_state_base* state) noexcept {
        move_it(std::move(x), state);
    }
    void clear() noexcept {
        if (_promise) {
            detach_promise();
        }
    }
    ~future_base() noexcept {
        clear();
    }
    promise_base* detach_promise() noexcept {
        _promise->_state = nullptr;
        _promise->_future = nullptr;
        return std::exchange(_promise, nullptr);
    }
    friend class promise_base;
};

template <typename Promise, typename... T>
task* continuation_base_with_promise<Promise, T...>::waiting_task() noexcept {
    return _pr.waiting_task();
}
template <typename... T>
struct future : public future_base {
    future_state<T...> _state;

    future(future_for_get_promise_marker m) noexcept {}
    future(promise<T...>* pr) noexcept : future_base(pr, &_state), _state(std::move(pr->_local_state)) {}
    template <typename... A>
    future(ready_future_marker m, A&&... a) noexcept : _state(m, std::forward<A>(a)...) {}
    explicit future(future_state<T...>&& state) noexcept : _state(std::move(state)) {}
    promise_base_with_type<T...> get_promise() noexcept {
        assert(!_promise);
        return promise_base_with_type<T...>(this);
    }
    promise_base_with_type<T...>* detach_promise() {
        return static_cast<promise_base_with_type<T...>*>(future_base::detach_promise());
    }
    template <typename Pr, typename Func>
    void schedule(Pr&& pr, Func&& func) noexcept {
        if (_state.available()) {
            ::seastar::schedule(new continuation<Pr, Func, T...>(std::move(pr), std::move(func), std::move(_state)));
        } else {
            assert(_promise);
            detach_promise()->schedule(std::move(pr), std::move(func));
            _state._u.st = future_state_base::state::invalid;
        }
    }
    future_state<T...>&& get_available_state_ref() noexcept {
        if (_promise) {
            detach_promise();
        }
        return std::move(_state);
    }
    template <typename... U>
    friend class shared_future;

    using value_type = std::tuple<T...>;
    using promise_type = promise<T...>;
    future(future&& x) noexcept : future_base(std::move(x), &_state), _state(std::move(x._state)) {}
    future(const future&) = delete;
    future& operator=(future&& x) noexcept {
        clear();
        move_it(std::move(x), &_state);
        _state = std::move(x._state);
        return *this;
    }
    void operator=(const future&) = delete;
    std::tuple<T...>&& get() {
        if (!_state.available()) {
            assert(0);
        }
        return get_available_state_ref().take();
    }
    void wait() noexcept {
        if (!_state.available()) {
            assert(0);
        }
    }
    bool available() const noexcept {
        return _state.available();
    }
    bool failed() const noexcept {
        return _state.failed();
    }
    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
    Result then(Func&& func) noexcept {
        return then_impl(std::move(func));
    }
    template <typename Func, typename Result>
    Result then_impl_nrvo(Func&& func) noexcept {
        using futurator = futurize<std::result_of_t<Func(T && ...)>>;
        typename futurator::type fut(future_for_get_promise_marker{});

        using pr_type = decltype(fut.get_promise());
        schedule(fut.get_promise(), [func = std::forward<Func>(func)](pr_type& pr, future_state<T...>&& state) mutable {
            if (state.failed()) {
                assert(0);
            } else {
                futurator::satisfy_with_result_of(std::move(pr), [&func, &state] {
                    return std::apply(func, std::move(state).get_value());
                });
            }
        });

        return fut;
    }
    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
    Result then_impl(Func&& func) noexcept {
        using futurator = futurize<std::result_of_t<Func(T && ...)>>;
        return then_impl_nrvo<Func, Result>(std::forward<Func>(func));
    }
    void forward_to(promise_base_with_type<T...>&& pr) noexcept {
        if (_state.available()) {
            pr.set_urgent_state(std::move(_state));
        } else {
            *detach_promise() = std::move(pr);
        }
    }
    void forward_to(promise<T...>&& pr) noexcept {
        if (_state.available()) {
            pr.set_urgent_state(std::move(_state));
        } else if (&pr._local_state != pr._state) {
            *detach_promise() = std::move(pr);
        }
    }
    void ignore_ready_future() noexcept {
        _state.ignore();
    }
    void set_coroutine(task& coroutine) noexcept {
        assert(!_state.available());
        assert(_promise);
        detach_promise()->set_coroutine(_state, coroutine);
    }
};
inline promise_base::promise_base(future_base* future, future_state_base* state) noexcept
    : _future(future), _state(state) {
    _future->_promise = this;
}
template <typename... T>
inline future<T...> promise<T...>::get_future() noexcept {
    assert(!this->_future && this->_state && !this->_task);
    return future<T...>(this);
}
template <typename... T, typename... A>
inline future<T...> make_ready_future(A&&... value) noexcept {
    return future<T...>(ready_future_marker(), std::forward<A>(value)...);
}
template <typename Func>
void futurize<void>::satisfy_with_result_of(promise_base_with_type<>&& pr, Func&& func) {
    func();
    pr.set_value();
}
template <typename... Args>
template <typename Func>
void futurize<future<Args...>>::satisfy_with_result_of(promise_base_with_type<Args...>&& pr, Func&& func) {
    func().forward_to(std::move(pr));
}
template <typename... T>
struct coroutine_traits_base {
    struct promise_type final : public seastar::task {
        seastar::promise<T...> _promise;

        promise_type() = default;
        promise_type(promise_type&&) = delete;
        promise_type(const promise_type&) = delete;
        template <typename... U>
        void return_value(U&&... value) {
            _promise.set_value(std::forward<U>(value)...);
        }
        void return_value(future<T...>&& fut) noexcept {
            fut.forward_to(std::move(_promise));
        }
        void unhandled_exception() noexcept {
            assert(0);
        }
        seastar::future<T...> get_return_object() noexcept {
            return _promise.get_future();
        }
        SEASTAR_INTERNAL_COROUTINE_NAMESPACE::suspend_never initial_suspend() noexcept {
            return {};
        }
        SEASTAR_INTERNAL_COROUTINE_NAMESPACE::suspend_never final_suspend() noexcept {
            return {};
        }
        virtual void run_and_dispose() noexcept override {
            auto handle = SEASTAR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<promise_type>::from_promise(*this);
            handle.resume();
        }
        task* waiting_task() noexcept override {
            return _promise.waiting_task();
        }
    };
};
template <>
struct coroutine_traits_base<> {
    struct promise_type final : public seastar::task {
        seastar::promise<> _promise;

        promise_type() = default;
        promise_type(promise_type&&) = delete;
        promise_type(const promise_type&) = delete;
        void return_void() noexcept {
            _promise.set_value();
        }
        void unhandled_exception() noexcept {
            assert(0);
        }
        seastar::future<> get_return_object() noexcept {
            return _promise.get_future();
        }
        SEASTAR_INTERNAL_COROUTINE_NAMESPACE::suspend_never initial_suspend() noexcept {
            return {};
        }
        SEASTAR_INTERNAL_COROUTINE_NAMESPACE::suspend_never final_suspend() noexcept {
            return {};
        }
        virtual void run_and_dispose() noexcept override {
            auto handle = SEASTAR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<promise_type>::from_promise(*this);
            handle.resume();
        }
        task* waiting_task() noexcept override {
            return _promise.waiting_task();
        }
    };
};
template <typename... T>
struct awaiter {
    seastar::future<T...> _future;

    explicit awaiter(seastar::future<T...>&& f) noexcept : _future(std::move(f)) {}
    awaiter(const awaiter&) = delete;
    awaiter(awaiter&&) = delete;
    bool await_ready() const noexcept {
        return _future.available();
    }
    template <typename U>
    void await_suspend(SEASTAR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<U> hndl) noexcept {
        _future.set_coroutine(hndl.promise());
    }
    std::tuple<T...> await_resume() {
        return _future.get();
    }
};
template <typename T>
struct awaiter<T> {
    seastar::future<T> _future;

    explicit awaiter(seastar::future<T>&& f) noexcept : _future(std::move(f)) {}
    awaiter(const awaiter&) = delete;
    awaiter(awaiter&&) = delete;
    bool await_ready() const noexcept {
        return _future.available();
    }
    template <typename U>
    void await_suspend(SEASTAR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<U> hndl) noexcept {
        _future.set_coroutine(hndl.promise());
    }
    T await_resume() {
        return _future.get0();
    }
};
template <>
struct awaiter<> {
    seastar::future<> _future;

    explicit awaiter(seastar::future<>&& f) noexcept : _future(std::move(f)) {}
    awaiter(const awaiter&) = delete;
    awaiter(awaiter&&) = delete;
    bool await_ready() const noexcept {
        return _future.available();
    }
    template <typename U>
    void await_suspend(SEASTAR_INTERNAL_COROUTINE_NAMESPACE::coroutine_handle<U> hndl) noexcept {
        _future.set_coroutine(hndl.promise());
    }
    void await_resume() {
        _future.get();
    }
};

template <typename... T>
auto operator co_await(future<T...> f) noexcept {
    return awaiter<T...>(std::move(f));
}
struct reactor {
    std::vector<task*> _task_queue;

    int run();

    void add_task(task* t) noexcept {
        _task_queue.push_back(std::move(t));
    }
};
extern __thread reactor* local_engine;
inline reactor& engine() {
    return *local_engine;
}
int reactor::run() {
    auto& tasks = _task_queue;
    while (!tasks.empty()) {
        auto tsk = tasks.back();
        tasks.pop_back();
        tsk->run_and_dispose();
    }
    return 0;
}
void promise_base::move_it(promise_base&& x) noexcept {
    _task = x._task;
    x._task = nullptr;
    _state = x._state;
    x._state = nullptr;
    _future = x._future;
    if (auto* fut = _future) {
        fut->detach_promise();
        fut->_promise = this;
    }
}

template <promise_base::urgent Urgent>
void promise_base::make_ready() noexcept {
    if (_task) {
        _state = nullptr;
        ::seastar::schedule(std::exchange(_task, nullptr));
    }
}
template void promise_base::make_ready<promise_base::urgent::no>() noexcept;
template void promise_base::make_ready<promise_base::urgent::yes>() noexcept;
promise_base& promise_base::operator=(promise_base&& x) noexcept {
    clear();
    move_it(std::move(x));
    return *this;
}
void promise_base::clear() noexcept {
    if (_future) {
        assert(_state);
        assert(_state->available() || !_task);
        _future->detach_promise();
    } else if (__builtin_expect(bool(_task), false)) {
        assert(0);
    }
}
promise_base::promise_base(promise_base&& x) noexcept {
    move_it(std::move(x));
}
void schedule(task* t) noexcept {
    engine().add_task(t);
}
thread_local std::unique_ptr<reactor> reactor_holder;
void allocate_reactor() {
    reactor_holder.reset(new reactor());
    local_engine = reactor_holder.get();
}
__thread reactor* local_engine;
}
namespace SEASTAR_INTERNAL_COROUTINE_NAMESPACE {
template <typename... T, typename... Args>
class coroutine_traits<seastar::future<T...>, Args...> : public seastar::coroutine_traits_base<T...> {};
}

using namespace seastar;

void foo() {}

future<> func() {
    future<> xyz = make_ready_future<>().then(foo);
    co_await std::move(xyz);
}

void exit0() {
    _exit(0);
}

future<> bar() {
    return func().then(exit0);
}

int main(int argc, char** argv) {
    allocate_reactor();
    make_ready_future<>().then(bar);
    engine().run();
    return 0;
}
