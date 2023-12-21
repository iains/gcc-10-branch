// { dg-options "-w -ansi -pedantic" }

// Copyright (C) 2003 Free Software Foundation, Inc.
// Contributed by Nathan Sidwell 23 Oct 2003 <nathan@codesourcery.com>

extern "Java" {
  class One 
  {
    ~One (); // { dg-error "cannot have a destructor" "" }
    One ();
  };

  class Two {};

  // We no longer seem to try to add an implicit dtor for this case, so that
  // the test is never made.
  // FIXME: we should try to find somewhere else to check that the class has
  // no implicit non-trivial DTORs.
  class Three : One {}; // { dg - error "cannot have an implicit" "" }

  class Four : Two {};

  class Five : Two, Four {}; //  { dg-error "cannot have multiple bases" "" }

  class Six : virtual Two {}; // { dg-error "cannot have virtual base" "" }
}
