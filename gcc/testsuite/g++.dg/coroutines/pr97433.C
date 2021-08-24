//  { dg-do run }

#include <iostream>
#include <optional>
#include <memory>
#include <utility>

#if defined(__clang__)
#include <experimental/coroutine>
namespace std
{
    template <typename T = void>
    using coroutine_handle = std::experimental::coroutine_handle<T>;
}
#elif defined(__GNUC__)
#include <coroutine>
#endif
using default_handle = std::coroutine_handle<>;

struct SuspendAlways
{
    static bool await_ready() noexcept { return false; }
    static void await_suspend(default_handle) noexcept {}
    static void await_resume() noexcept {}
};

template <typename ... Args>
void RawWriteLine(Args const & ... args) noexcept
{
	((std::cout << args), ...);
	std::cout << "\n";
}

template <typename ... Args>
void Log(Args const & ... args) noexcept
{
	std::cout << "LOG\t";
	((std::cout << args), ...);
	std::cout << "\n";
}

template <typename ... Args>
void WriteLine(Args const & ... args) noexcept
{
    std::cout << "\t";
	((std::cout << args), ...);
	std::cout << "\n";
}

template <typename ... Args>
void Assert(bool value, Args const & ... args) noexcept
{
	RawWriteLine(value ? "PASS\t" : "FAIL\t", args...);
}
#define ASSERT(result, expected) Assert(result == expected, __func__, ":", __LINE__, " ", #result " == " #expected, "\n\t[ result = ", result, ", expected = ", expected, " ]")
#define LOG_MEM_FN() WriteLine(__func__, ": ", this)

default_handle m_current = nullptr;

struct None{};

struct Continuable
{
private:
	default_handle m_continuation = nullptr;
public:
	void SetCurrent(default_handle handle) noexcept
	{
		LOG_MEM_FN();
		m_current = handle;
	}
	default_handle ContinueWith(default_handle handle = nullptr) noexcept
	{
		LOG_MEM_FN();
		return std::exchange(m_continuation, handle);
	}
};

struct YieldAwaitable
{
	Continuable & m_promise;

	bool await_ready() const noexcept
	{
		WriteLine("YieldAwaitable: ", __func__, ": ", &m_promise);
		return false;
	}

	auto await_suspend(default_handle) const noexcept
	{
		WriteLine("YieldAwaitable: ", __func__, ": ", &m_promise);
		return m_promise.ContinueWith();
	}

	void await_resume() const noexcept
	{
		WriteLine("YieldAwaitable: ", __func__, ": ", &m_promise);
	}
};

struct RootAwaitable : SuspendAlways
{
	template <typename T>
	RootAwaitable(T &&){}
};

template <typename Promise>
struct AwaitAwaitable
{
	Promise & m_promise;

	bool await_ready() const noexcept
	{
		WriteLine("AwaitAwaitable: ", __func__, ": ", &m_promise);
		return false;
	}

	void await_suspend(default_handle handle) noexcept
	{
		WriteLine("AwaitAwaitable: ", __func__, ": ", &m_promise);
		m_promise.SetCurrent( m_promise.Handle() );
		m_promise.ContinueWith( handle );
	}

	auto await_resume() const noexcept
	{
		WriteLine("AwaitAwaitable: ", __func__, ": ", &m_promise);
		return m_promise.GetValue();
	}
};

template <typename P>
AwaitAwaitable(P)->AwaitAwaitable<P>;

template <typename T>
struct Settable;

struct SettableHasValue : Continuable
{
private:
	bool m_has_value = false;
public:
	bool HasValue() const noexcept { return m_has_value; }
	void SetValue() noexcept { m_has_value = true;  }
	void GetValue() noexcept { m_has_value = false; }
};

template <>
struct Settable<void> : SettableHasValue
{
	auto yield_value(None) noexcept -> YieldAwaitable
	{
		LOG_MEM_FN();
		SetValue();
		return {*this};
	}
	void return_void() noexcept 
	{ 
		LOG_MEM_FN();
		SetValue();
	}
};

template <typename T>
struct Settable : SettableHasValue
{
private:
	using base = SettableHasValue;
	union { T m_value; };

    void Construct(T const & value) noexcept
    {
		new (&m_value) T{ value };
		base::SetValue();
    }

    void Destruct() noexcept
    {
		LOG_MEM_FN();
        m_value.~T();
        base::GetValue();
    }
public:
	void SetValue(T const & value) noexcept
	{
		LOG_MEM_FN();
        Destruct();
		Construct(value);
	}

	T GetValue() noexcept
	{
		LOG_MEM_FN();
		T output{ std::move(m_value) };
		Destruct();
		return output;
	}

	auto yield_value(T const & value) noexcept -> YieldAwaitable
	{
		LOG_MEM_FN();
		SetValue(value);
		return {*this}; 
	}

	void return_value(T const & value) noexcept
	{
		LOG_MEM_FN();
		SetValue(value);
	}
};

template <typename T, typename FinalAwaitable = YieldAwaitable>
struct Task : private default_handle
{
	struct promise_type : Settable<T>
	{
		auto Handle() noexcept
		{
			return handle_type::from_promise(*this);
		}

		Task<T, FinalAwaitable> get_return_object() noexcept
		{
			LOG_MEM_FN();
			return { Handle() };
		}

		SuspendAlways initial_suspend() noexcept
		{
			LOG_MEM_FN();
			return {};
		}

		FinalAwaitable final_suspend() noexcept
		{
			LOG_MEM_FN();
			return { *this };
		}

		void unhandled_exception() noexcept
		{
			LOG_MEM_FN();
			std::terminate();
		}

		template <typename Tsk>
		auto await_transform( Tsk && tsk ) noexcept
		{
			LOG_MEM_FN();
			return AwaitAwaitable{ std::forward<Tsk>(tsk).Promise() };
		}

		~promise_type()
		{
			LOG_MEM_FN();
		}
	};

	using handle_type = std::coroutine_handle<promise_type>;

	handle_type & Handle() noexcept { return reinterpret_cast<handle_type&>(*this); }
	
    handle_type const & Handle() const noexcept { return reinterpret_cast<handle_type const &>(*this); }

	promise_type & Promise() noexcept { return Handle().promise(); }
	
    promise_type const & Promise() const noexcept { return Handle().promise(); }

	handle_type Detach() noexcept
	{
		return std::exchange(Handle(), nullptr);
	}

	void Destroy() noexcept
	{
		if ( Handle() )
		{
			WriteLine( __func__, ": ", &Promise() );
			Handle().destroy();
		}
	}

	Task() noexcept : default_handle{nullptr}{}
    
	Task(default_handle handle) noexcept :
		default_handle{handle}
	{}

	Task(Task && task) noexcept :
		default_handle
        {
            std::exchange(std::move(task).Handle(), nullptr)
        }
	{
		WriteLine( __func__, ": ", &Promise() );
	}

	Task & operator=(Task && task) noexcept
	{
		WriteLine( __func__, ": ", &Promise() );
		Destroy();
		Handle() = std::exchange(std::move(task).Handle(), nullptr);
		return *this;
	}

	Task(Task const & task) noexcept = delete;

	Task & operator=(Task const & task) noexcept = delete;

	~Task()
	{ 
		Destroy();
		Handle() = nullptr;
	}
};

Task<int> test_coro0()
{
    Log("Test test_coro0");
	co_return 123;
}

Task<int> test_coro1()
{
	co_return co_await test_coro0();
}

Task<int> test_coro2()
{
    Log("Test test_coro2");
	co_return co_await test_coro1();
}

Task<int> test_yielding()
{
    Log("Test test_yielding");
	co_yield 1;
	co_return 2;
}

Task<void> test_task() noexcept
{
    Log("Test co_yield + co_return rvalue");
    auto fn = test_yielding();
    auto res =
    (
        co_await fn +
        co_await fn
    );
    ASSERT(res, 3);
}

Task<void, RootAwaitable> Runner() noexcept
{
    Log("Test Runner");
	co_await test_task();
}

void runner() noexcept
{
  auto run = Runner();
  m_current = run.Handle();
  while(m_current &&  not m_current.done())
    {
        m_current.resume();
        Log("<tick>");
    }
}

// Run perfectly in LLVM compiler in godbolt, GCC and Clang
// Works LLVM : https://godbolt.org/z/ha54xP
// Fails GCC  : https://godbolt.org/z/hPcxv9
// Fail MSVC  : 
int main()
{
	runner();
	return 0;
}