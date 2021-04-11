# Branches from GCC 10.3.0 for Darwin/Mach-O 

Iain Sandoe, April 2021

## Preface

### *WARNING* *WARNING* *WARNING* *WARNING*

**These branches are experimental**

### Processing issues and recording changes.

We can use github issues for working on this, it's not going to be appropriate
in the to file GCC bugzillas.

There is no ChangLog; git commit messages should be made sensible (or silly in
a suitably informative manner).

### Document Changes

It might be worth tracking changes in understanding - but we shall see,
git commit messages can work for that too.

2021-04-10 Imported and back-ported changes from master


### gcc-10-3-darwin : Branch Additional Patches

The following patches have been back-ported or applied preemptively to fix outstanding bugs or improve the compatibility on Darwin.

* Darwin, configury : Allow for specification and detection of dsymutil.
  This fixes a long-standing problem (actually from when dsymutil was originally added), where the rules for finding target tools are not applied to dsymutil.  Often we can 'get away with it' because the system installation is close enough.  However that fails if we include an arch that's not covered by the system tool.  This patch treats dsymutil in the same way as 'ld' and 'as' since it needs to match the choices for those tools.

* Handle embedded run-paths (@rpath / rpath) [PR88590]
  - Driver: Provide a spec to insert rpaths for compiler lib dirs.
  - Darwin : Handle rpaths given on the command line.
  - Darwin : Allow for configuring Darwin to use embedded runpath.
  - Darwin, Ada : Add loader path as a default rpath.
  - configure, Darwin : Adjust ```RPATH_ENVVAR``` for embedded run-paths.

  These are really one patch, but kept separate for now so that I can keep things in sync between master and the branch.  This changes the way in which target runtimes are built so that they are named ```@rpath/libxxxx.dylib```.  It also adds a default rpath set to the output of GCC that names the installation directories.  This looks to the user the same as before (the libraries from the compiler install are used by default).  However it has three important benefits.
 1.  This means that we can drop the use of ```DYLD_LIBRARY_PATH``` at build time and make sure that we inject the correct paths for the build.
 2.  Actually, it means that testing will now work without installing the target libraries first.
 3.  It simplifies the packaging of libraries in application packages (since the compiler already builds them in a suitable form).

  We disable the automatic injection of embedded run paths (rpath) if there is any -rpath or -Wl,-rpath on the command line, so that the user can override.
  
  It is safe to install @rpath/libxxxx.dylib in place of libraries which previously were refered to by absolute paths.  Any exe that refers to those will still find the libraries at the expected path.
  
  This facility is available from Darwin9+ (has to be enabled specifically).

  It is defaulted on from Darwin15, since ```DYLD_LIBRARY_PATH``` is not functional from then.

  **This is needed to allow the build of the revised libgcc_s below.**

* libiberty, Darwin : Fix simple-object LTO table for cross-endian case.
  Bug fix for cross-endian LTO (fixes an ICE).

* Darwin: Rework handling for unwinder code in libgcc_s and specs [PR80556].

  This revises the handling of the unwinder to resolve the remaining pieces of this long-standing issue.

* Darwin : Mark the mod init/term section starts with a linker-visible sym.
  XCode assemblers insert temp symbols for the start of init or term sections, these are linker-visible and then lead to debug-compare mismatches because the number associated with each of these changes when debug is enabled.

* Darwin : Use a reserved name for the exception tables sect start.
  Housekeeping, we should use an implementation-space name for this to avoid conflicts.
  
* CFI-handling : Add a hook to allow target-specific Personality and LSDA indirections
  This back-ports the change to allow us to support .cfi_personality (IT SHOULD NOT BE ENABLED YET, SINCE there are still issues with compact unwind).  Optional, I guess.
  
* configury : Fix LEB128 support for non-GNU assemblers.
 Optional back-port.
 
* C++ : Add the -stdlib= option.
  Optional, but highly recommended, back-port.  On systems where the system stdc++ lib is libc++ this allows GCC to use -stdlib=libc++ to emit code and link the libc++ instead of libstdc++.  This can be very useful (but it requires some self-assembly; the provision of a suitable set of libc++ headers) - there's no way that these can be included in the patch, of course.

* Darwin, libgcc : Adjust min version supported for the OS.
  Newer ld64 complain about old system versions being linked - this causes build and test fails for no good reason.  The patch causes the ```crts``` and ```libgcc_s``` to be built for the oldest supported at the given configured case.  NOTE: you still need to use ```MACOSX_DEPLOYMENT_TARGET``` to make all the target libs follow this pattern (however this generally won't affect people just building / testing on self-hosted cases).


