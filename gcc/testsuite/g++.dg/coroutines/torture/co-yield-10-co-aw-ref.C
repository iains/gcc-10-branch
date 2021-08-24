//  { dg-do run }

/* The simplest valued co_await we can do.  */

#include "../coro.h"

// boiler-plate for tests of codegen
#include "../coro1-ret-int-yield-int.h"

coro1
f ()
{
  int t = 5;
  co_yield co_await coro1::suspend_always_intrefprt{t};
  co_return t + 10;
}

int main ()
{
  PRINT ("main: create coro1");
  struct coro1 f_coro = f ();

  PRINT ("main: resuming [1] initial suspend");
  f_coro.handle.resume();
  PRINT ("main: resuming [2] co_await suspend_always_intprt");
  f_coro.handle.resume();

  int y = f_coro.handle.promise().get_value();
  if (y != 5)
    {
      PRINTF ("main: y is wrong : %d, should be 5\n", y);
      abort ();
    }
  PRINT ("main: resuming [3] co_yield ");
  f_coro.handle.resume();

  /* we should now have returned with the co_return (15) */
  if (!f_coro.handle.done())
    {
      PRINT ("main: we should be 'done' ");
      abort ();
    }
  y = f_coro.handle.promise().get_value();
  if (y != 15)
    {
      PRINTF ("main: y is wrong : %d, should be 15\n", y);
      abort ();
    }
  puts ("main: done");
  return 0;
}
