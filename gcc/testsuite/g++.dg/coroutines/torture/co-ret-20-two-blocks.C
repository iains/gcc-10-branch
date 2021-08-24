//  { dg-do run }

// Test returning an int.
// We will use the promise to contain this to avoid having to include
// additional C++ headers.

#include "../coro.h"

// boiler-plate for tests of codegen
#include "../coro1-ret-int-yield-int.h"

struct coro1
f (int v)
{
  {
    if (v)
      co_return 42;
  }
  {
    int p = 10;
    co_return p;
  }
}

int main ()
{
  PRINT ("main: create coro1");
  struct coro1 x = f (10);
  PRINT ("main: got coro1 - resuming");
  if (x.handle.done())
    abort();
  x.handle.resume();
  PRINT ("main: after resume");
  int y = x.handle.promise().get_value();
  if ( y != 42 )
    abort ();
  if (!x.handle.done())
    {
      PRINT ("main: apparently not done...");
      abort ();
    }
  PRINT ("main: returning");
  return 0;
}
