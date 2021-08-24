// { dg-do run }

// Test co-await in if condition.

#include "../coro.h"

// boiler-plate for tests of codegen
#include "../coro1-ret-int-yield-int.h"

/* An awaiter that suspends always and returns a boolean as the
   await_resume output.  */
struct BoolAwaiter {
  bool v;
  BoolAwaiter (bool _v) : v(_v) {}
  bool await_ready () { return false; }
  void await_suspend (coro::coroutine_handle<>) {}
  bool await_resume () { return v; }
};

struct Fun {
  int a;
  Fun (int x) : a(x) {}
 ~Fun () {}
  int value (){ return a; }
};

/* An awaiter that suspends always and returns an int as the
   await_resume output.  */
struct IntAwaiter {
  int v;
  IntAwaiter (int _v) : v(_v) {}
  bool await_ready () { return false; }
  void await_suspend (coro::coroutine_handle<>) {}
  int await_resume () { return v; }
};

/* An awaiter that suspends always and returns an int as the
   await_resume output.  */
struct FunIntAwaiter {
  int v;
  FunIntAwaiter (int _v) : v(_v) {}
  bool await_ready () { return false; }
  void await_suspend (coro::coroutine_handle<>) {}
  Fun await_resume () { return Fun{v}; }
};

//extern int tt ();
  int three = 3;
  int two = 2;

int bar (Fun&& v)
{
  return v.value();
}

int baz (bool t)
{
  int a = 2,c;
#if 1
  int&& v = t ? bar (Fun (a + 5)) : bar (Fun (a - 4));
  printf ("v = %d\n", v);
  return v;
#else
  int&& x = (x = t ? bar (Fun (a + 5)) : bar (Fun (a - 4)), c = x - 1, std::move (c));
  printf ("x = %d\n", x);
  return x;
#endif
}

struct coro1
my_coro (bool t) noexcept
{
  int a = 2,c;
  bool tt;
  bool ttt;
//  Fun x;
//  c = t ? bar (Fun (a + 5)) : bar (Fun (a - 4));
  c = a + (t ? Fun(a + 5) : Fun (a / 6)).value();
#if 0
  tt = c > 5;
  ttt = c > 22;
//  IntAwaiter aw = t ? Fun (a), IntAwaiter (4) : IntAwaiter (5);
  IntAwaiter aw = t ? tt ? IntAwaiter (4) : IntAwaiter (5) : tt ? IntAwaiter (6) : IntAwaiter (7);
  IntAwaiter aw0 (0);
  t ? aw0 = IntAwaiter(4) : aw0 = IntAwaiter(5);
  auto aw1 (t ? IntAwaiter (4) : IntAwaiter (5));
  auto aw2 {t ? IntAwaiter (4) : IntAwaiter (5)};
  auto aw3 = (Fun(1), (t ? IntAwaiter (4) : IntAwaiter (5)));
#endif
  //c = t ? co_await IntAwaiter (4) : co_await IntAwaiter (5);
//  int&& x = (x = t ? bar (Fun (a + 5)) : bar (Fun (a - 4)), c = x - 1, std::move (c));
//  printf ("x = %d\n", x);
//  c = 
//  baz (t);
//  Fun b(9);
//  t ? b = Fun(10) : b = Fun{11}, a = b.value();

  static_cast<void>(0), 
 a = !(__builtin_expect(!((co_await FunIntAwaiter (t)).value()), 0))
? c + 6165 
: 42;// int a = co_await BoolAwaiter (t) ? 6174 : 42;

  co_return a;
}

int main ()
{
  PRINT ("main: create coro");
  struct coro1 x = my_coro (true);

//  baz (false);
  if (x.handle.done())
    {
      PRINT ("main: apparently done when we should not be...");
      abort ();
    }

  PRINT ("main: resume initial suspend");
  x.handle.resume();

  PRINT ("main: if condition 1 - true");
  x.handle.resume();

//  PRINT ("main: if condition 2 - true");
//  x.handle.resume();

  PRINT ("main: after resume");
  int y = x.handle.promise().get_value();
  if ( y != 6174 )
    {
      PRINTF ("main: apparently wrong value : %d\n", y);
      abort ();
    }

  if (!x.handle.done())
    {
      PRINT ("main: apparently not done...");
      abort ();
    }
  PRINT ("main: returning");
  return 0;
}
