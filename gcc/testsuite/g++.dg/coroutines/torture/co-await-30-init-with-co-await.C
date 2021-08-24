//  { dg-do run }

/* The simplest valued co_await we can do.  */

#include "../coro.h"

// boiler-plate for tests of codegen
#include "../coro1-ret-int-yield-int.h"

int foo (int&& x)
{
  return x + 5;
}

int bar (int&& x, int y)
{
  return x + y;
}

coro1
my_coro ()
{
  int t1 = co_await coro1::suspend_always_intprt{};
  int t2 = foo (co_await coro1::suspend_always_intprt{});
  int t3 = bar (co_await coro1::suspend_always_intprt{},
		co_await coro1::suspend_always_intprt{});
  co_return t1 + t2 + t3;
}

int main ()
{
  PRINT ("main: create coro1");
  struct coro1 f_coro = my_coro ();
  if (f_coro.handle.done())
    {
      PRINT ("main: we should not be 'done' [1]");
      abort ();
    }
  PRINT ("main: resuming [1] initial suspend");
  f_coro.handle.resume();
  PRINT ("main: resuming [2] co_await suspend_always_intprt");
  f_coro.handle.resume();
  PRINT ("main: resuming [3] foo(co_await suspend_always_intprt)");
  f_coro.handle.resume();
  PRINT ("main: resuming [4,5] bar(co_await suspend_always_intprt, co_await suspend_always_intprt)");
  f_coro.handle.resume();
  f_coro.handle.resume();
  /* we should now have returned with the co_return (15) */
  if (!f_coro.handle.done())
    {
      PRINT ("main: we should be 'done' ");
      abort ();
    }
  int y = f_coro.handle.promise().get_value();
  if (y != 25)
    {
      PRINTF ("main: y is wrong : %d, should be 15\n", y);
      abort ();
    }
  PRINT ("main: done");
  return 0;
}
