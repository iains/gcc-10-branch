//  { dg-do run }

// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../coro.h"
#include <cassert>

//using namespace std::experimental;
using namespace coro;

bool cancel = false;

struct goroutine
{
  static int const N = 100;
  static int count;
  static coroutine_handle<> stack[N];

  static void schedule(coroutine_handle<>& rh)
  {
    assert(count < N);
    stack[count++] = rh;
    rh = nullptr;
  }

  ~goroutine() { /*puts ("goro dtor");*/}

  static void go (goroutine) {}

  static void run_one()
  {
    assert(count > 0);
    stack[--count]();
  }

  struct promise_type
  {
    suspend_never initial_suspend() {
      return {};
    }
    suspend_always final_suspend() noexcept {
      return {};
    }
    void return_void() {}
    goroutine get_return_object() {
      return{};
    }
    void unhandled_exception() {}
  };
};

int goroutine::count = 0;
coroutine_handle<> goroutine::stack[goroutine::N];

coroutine_handle<goroutine::promise_type> workaround;

class channel;

struct push_awaiter {
  channel* ch;

  bool await_ready() noexcept { return false; }
  void await_suspend(coroutine_handle<> rh) noexcept;
  void await_resume() noexcept;
};

struct pull_awaiter {
  channel * ch;

  bool await_ready() noexcept ;
  void await_suspend(coroutine_handle<> rh) noexcept;
  int await_resume() noexcept;
};

class channel
{
  using T = int;

  friend struct push_awaiter;
  friend struct pull_awaiter;

  T const* pvalue = nullptr;
  coroutine_handle<> reader = nullptr;
  coroutine_handle<> writer = nullptr;

public:

  push_awaiter push(T const& value)
  {
    assert(pvalue == nullptr);
    assert(!writer);
    pvalue = &value;
  //printf ("push() %d, [%p]\n", value, pvalue);
    return { this };
  }

  pull_awaiter pull()
  {
    assert(!reader);
  //printf ("pull()\n");
    return { this };
  }

  void sync_push(T const& value)
  {
    assert(!pvalue);
    pvalue = &value;
  //printf ("sync_push() %d, [%p]\n", value, pvalue);
    assert(reader);
    reader();
    assert(!pvalue);
    reader = nullptr;
  }

  auto sync_pull()
  {
    while (!pvalue) goroutine::run_one();
    auto result = *pvalue;
    pvalue = nullptr;
    if (writer)
    {
      auto wr = writer;
      writer = nullptr;
      wr();
    }
    return result;
  }
};

void push_awaiter::await_suspend(coroutine_handle<> rh) noexcept
{
  ch->writer = rh;
  if (ch->reader) goroutine::schedule(ch->reader);
}

void push_awaiter::await_resume () noexcept
{
  // The value pointer must not escape from the full expression.
  assert (ch->pvalue == nullptr);
}

bool pull_awaiter::await_ready() noexcept
{
  return !!ch->writer;
}

void pull_awaiter::await_suspend(coroutine_handle<> rh) noexcept
{
  ch->reader = rh;
}

int pull_awaiter::await_resume() noexcept
{
  auto result = *ch->pvalue;
  ch->pvalue = nullptr;
  if (ch->writer) {
    //goroutine::schedule(ch->writer);
    auto wr = ch->writer;
    ch->writer = nullptr;
    wr();
  }
  return result;
}

goroutine pusher (channel& left, channel& right)
{
  for (;;) {
#if NO_LIFETIME_EXTENSION
    auto val = 1 + co_await left.pull();
    //printf ("pulled %d and added 1, pushing \n", val-1);
    co_await right.push(val);
#else
    auto val = co_await left.pull();
    //printf ("pulled %d, pushing val + 1\n", val);
    // NOTE: this is horrible, it absolutely depends on the lifetime extension
    // of the temporary "val + 1" to the full expression including the co_await.
    // The lifetime extension is not obvious here, you need to look at the
    // push routine signature.
    // Most refactoring would break this.
    co_await right.push(val+1);
#endif
  }
}

const int N = 100;
channel* c = new channel[N + 1];

int main() {
  //puts ("creating");
  for (int i = 0; i < N; ++i)
    goroutine::go (pusher(c[i], c[i + 1]));

  PRINT ("push 0");
  c[0].sync_push(0);
  
  PRINT ("pull N");
  int result = c[N].sync_pull();

  PRINTF ("result : %d\n", result);
  if (result != N)
    abort();

  PRINT ("push 3");
  c[0].sync_push(3);
  
  PRINT ("pull N");
  result = c[N-3].sync_pull();

  PRINTF ("result : %d\n", result);
  if (result != N)
    abort();
  return 0;
}
