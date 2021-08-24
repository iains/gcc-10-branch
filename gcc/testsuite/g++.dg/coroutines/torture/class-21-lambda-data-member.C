// { dg-do run { target c++20 } }

// template parm in a class

#include "../coro.h"

// boiler-plate for tests of codegen
#define USE_AWAIT_TRANSFORM
#include "../coro1-ret-int-yield-int.h"

template <typename T>
class foo
{
  int v;
  public:
  foo (int _x) : v(_x) {}
  auto get_lam (int parm)
    {
      int local = parm;
      auto l = [=, this](T y) -> coro1
      {
	T x = y;
	co_return co_await v + y + local;
      };
      return l;
    }
};

int main ()
{
  foo<int> inst(5);
  auto ll = inst.get_lam (10);

  PRINT ("main: create coro1");
  int arg = 17; // avoid a dangling reference
  coro1 x = ll (arg);
  if (x.handle.done())
    abort();

  x.handle.resume();
  PRINT ("main: after resume (initial suspend)");

  x.handle.resume();
  PRINT ("main: after resume (co_await)");

  /* Now we should have the co_returned value.  */
  int y = x.handle.promise().get_value();
  if ( y != 32 )
    {
      PRINTF ("main: wrong result (%d).", y);
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
