//  { dg-do run }

// Simplest class.

#include "../coro.h"

// boiler-plate for tests of codegen
#include "../coro1-ret-int-yield-int.h"
#include <utility>

class foo
{
  int t = 6174;
  public:

  coro1 coro_meth ()
    {
      PRINT ("coro1: about to return 42");
      co_return 42;
    }

  auto get_coro_lam () {
    auto coro_lam = [this] () -> coro1 
    {
      PRINT ("coro1: about to return 6174");
      co_return t;
    };
    return coro_lam;
  }

  auto get_coro_lam_pair () {
    auto coro_lam_a = [this] () -> coro1 
    {
      PRINT ("coro1: about to return 6174");
      co_return t;
    };
    auto coro_lam_b = [this] () -> coro1 
    {
      PRINT ("coro1: about to return 42");
      co_return 42;
    };
   return std::pair(coro_lam_a, coro_lam_b);
  }
};

int main ()
{
  foo inst;

  PRINT ("main: create coro1");
  coro1 x = inst.coro_meth ();
 
  auto ll = inst.get_coro_lam ();
  coro1 z = ll ();

  PRINT ("main: got coro1 - resuming");
  if (x.handle.done())
    abort();

  PRINT ("main: coro (initial suspend)");
  x.handle.resume();

  int y = x.handle.promise().get_value();
  if ( y != 42 )
    abort ();
  if (!x.handle.done())
    {
      PRINT ("main: apparently not done...");
      abort ();
    }

  if (z.handle.done())
    abort();

  PRINT ("main: lambda (initial suspend)");
  z.handle.resume();
  y = z.handle.promise().get_value();
  if ( y != 6174 )
    abort ();

  if (!z.handle.done())
    {
      PRINT ("main: apparently not done...");
      abort ();
    }

  PRINT ("main: returning");
  return 0;
}
