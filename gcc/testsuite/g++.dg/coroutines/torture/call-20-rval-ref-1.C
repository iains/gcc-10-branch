// { dg-do run }

// Check  foo (compiler temp, co_await).

#include "../coro.h"

// boiler-plate for tests of codegen
//#define USE_AWAIT_TRANSFORM
#include "../coro1-ret-int-yield-int.h"

struct test
{
  const char *p;
  test () : p("void ctor") { PRINT("called test ctor"); }
  test (const char *v) : p(v) { PRINT("called test ctor with str");}
  test (const test &) = delete;
  test (test &&s) : p(s.p) { s.p = (const char*)0; PRINT("test used move ctor "); }
  const char *val () { return p; }

  auto operator co_await() & noexcept { 
    return coro1::suspend_always_intprt{};
  }

  auto operator co_await() && noexcept { 
    return coro1::suspend_always_longprtsq(3L);
  }
  
};

__attribute__((__noinline__))
void use_test (test&& v)
{
  PRINTF ("value = %s\n", v.val());
}
 __attribute__((__noinline__))
void use_testr (test& v)
{
  PRINTF ("value = %s\n", v.val());
}

/* Function with a compiler temporary and a co_await.  */
coro1
g (test x)
{
  //test t = std::move(x);
  //use_test (std::move(x));
  //use_test (static_cast<test&&>(x));
  co_yield co_await static_cast<test&&>(x);
  co_return 0;
}

int main ()
{
  PRINT ("main: create coro1");
  //test lv("lvalue");
  //coro1 g_coro = g (lv);
  coro1 g_coro = g (test("rvalue"));

  /* We should have created the promise using the void ctor which sets
     the contained value to -1.  */
  int y = g_coro.handle.promise().get_value();
  if (y != -1)
    {
      PRINTF ("main: gX is wrong : %d, should be -1\n", y);
      abort ();
    }

  PRINT ("main: resuming [1] (initial suspend)");
  g_coro.handle.resume();
  
  PRINT ("main: resuming [2] co_await");
  g_coro.handle.resume();

  y = g_coro.handle.promise().get_value();
  if (y != 9)
    {
      PRINTF ("main: y is wrong : %d, should be 9\n", y);
      abort ();
    }

  PRINT ("main: resuming [3] co_yield");
  g_coro.handle.resume();

  /* we should now have returned with the co_return 0) */
  if (!g_coro.handle.done())
    {
      PRINT ("main: we should be 'done'");
      abort ();
    }

  y = g_coro.handle.promise().get_value();
  if (y != 0)
    {
      PRINTF ("main: y is wrong : %d, should be 0\n", y);
      abort ();
    }

  puts ("main: done");
  return 0;
}