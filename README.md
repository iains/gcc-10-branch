# Branches from GCC 10.3.0 for Darwin/Mach-O 

Iain Sandoe, April 2021

## Preface

### *WARNING* *WARNING* *WARNING* *WARNING*

**The Arm64 branches are experimental**

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

### gcc-10-3-extra-backports : Branch Additional Patches

* aarch64: Remove redundant mult pattern

  Backported fix needed for build to succeed with LLVM assembler

* aarch64: Don't emit invalid zero/sign-extend syntax

  Backported fix needed for build to succeed with LLVM assembler

* fortran: ```caf_fail_image``` expects no argument

  Backported bug fix for wrong code gen.

* fortran: Fix function arg types for class objects

  Backported bug fix for wrong code gen.

* fortran: Fix arg types of ```_gfortran_is_extension_of```

  Backported bug fix for wrong code gen.

* fortran: Fix argument types in derived types procedures

  Backported bug fix for wrong code gen.

* config.sub, config.guess : Import upstream 2020-11-07.

  Recognise arm64-apple-darwin20

* aarch64 : Mark rotate immediates with '#' as per DDI0487iFc.

  Backported fix needed for build to succeed with LLVM assembler

### gcc-10-3-apple-si : Branch Additional Patches

2021-04-10 Imported and back-ported changes from master version master-wip-apple-si-on-a809d8a737da

## Introduction.

This is taken from:

[1] [ARM IHI 0055B : AAPCS64 (current through AArch64 ABI release 1.0, dated 22nd May 2013)](http://infocenter.arm.com/help/topic/com.arm.doc.ihi0055b/IHI0055B_aapcs64.pdf)

[2] [Apple iOS document : darwinpcs](https://developer.apple.com/library/archive/documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARM64FunctionCallingConventions.html#//apple_ref/doc/uid/TP40013702-SW1)

[3] [The LLVM backend for Mach-O/arm64 from GIT (master 12 at the document date).](https://github.com/llvm/llvm-project.git)

**Darwin** is the kernel used for macOS (10/OSX and 11) and iOS (all versions,
so far).

**Mach-O** is the file format used for object files and DSOs, including executables
on Darwin platforms (to some extent, the two names are interchangeable in
describing rules applicable to a back end).

The `AArch64` port for Darwin is known as `arm64` (`arm64` is synonymous with
`aarch64` for Darwin in GCC).   There is an ILP32 variant, `arm64_32` (not yet
considered in detail or handled by these branches).

There are two main technical sections 

* Part 1 which describes the darwinpcs deviations from AAPCS64.

   This is a primarily cross-reference between [2] and [1].

* Part 2 contains additional pertinent information.

   Some is recorded in [2] but most is determined from generic Darwin/Mach-O
   rules and the implementation [3].

**Note** Since a macOS or iOS system is built with a toolchain based on [3],
that sets the de facto ABI.  Therefore, the ABI as implemented by the XCode
version appropriate to a given OS release shall take precedence over version(s)
described in the referenced documents in the event of discrepancy.

## PART 1 - AAPCS64 and darwinpcs.

### Outline

Darwin PCS Differences from AAPCS64.

The intent of these notes are to match the differences described in [2] against
the section numbers and rule designations of [1] since the AArch64 port code
uses the rule designations in comments.

The organisation of these notes is by section heading of [1].

[2] Refers to the darwinpcs as "iOS" which was the first Darwin OS variant
implementing it, however it is stated (albeit unofficially?) that Arm64 macOS
will adopt the same ABI and is expected to be able to execute iOS executables.

So, for the present, 'iOS' is considered to be equivalent to 'macOS'
(generically 'Darwin').

In the text from [2] the expression "generic procedure call standard" refers
to the AAPCS64 [1].

Darwin PCS rules are designated `D.N` below.

### AAPCS64 Section 1.

No amendments.

### AAPCS64 Section 2.

No amendments.

### AAPCS64 Section 3.

No amendments.

The darwinpcs is non-conforming with the aapcs64 in the areas described below.

### AAPCS64 Section 4.

No amendments

but note:
* Darwin's `char` and `wchar_t` are both signed.
* Where applicable, the `__fp16 type` is `IEEE754-2008` format.

### AAPCS64 Section 5.

5.1 Machine Registers
5.1.1 General-purpose Registers

Darwin reserves `x18` as the platform register (as permitted).

5.2 Processes, Memory and the Stack
5.2.3 The Frame Pointer

From [2] : The frame pointer register (x29) must always address a valid frame
record, although some functions—such as leaf functions or tail calls—may elect
not to create an entry in this list.

This corresponds to the first bullet and is conforming.  It implies that Darwin
should warn if the user tries to use an option that omits the FP.

5.4 Parameter Passing
5.4.2 Parameter Passing Rules

`D.1` From [2] : Empty struct types are ignored for parameter-passing purposes.
This behavior applies to the GNU extension in C and, where permitted by the
language, in C++.

It is noted that this might not correspond to any specific rule - but,
presumably, needs to be applied in marshalling arguments.

`D.2` From [2] : In the generic procedure call standard, all function arguments
passed on the stack consume slots in multiples of 8 bytes. In iOS, this
requirement is dropped, and values consume only the space required.   Padding
is still inserted on the stack to satisfy arguments’ alignment requirements.

`D.3` From [2] : The general ABI specifies that it is the callee’s responsibility
to sign or zero-extend arguments having fewer than 32 bits, and that unused bits
in a register are unspecified. In iOS, however, the caller must perform such
extensions, up to 32 bits.  This apparently conflicts with the `D.2` above and
thus can only be applied to values passed in registers?

(notwithstanding C rules for widening).

`D.4` From [2] : The generic procedure call standard requires that arguments
with 16-byte alignment passed in integer registers begin at an even-numbered
xN, skipping a previous odd-numbered xN if necessary. The iOS ABI drops this
requirement.

#### Variadic Functions

From [2]:
The iOS ABI for functions that take a variable number of arguments is entirely
different from the generic version.

Stages A and B of the generic procedure call standard are performed as usual.
in particular, even variadic aggregates larger than 16 bytes are passed via a
reference to temporary memory allocated by the caller. After that, the fixed
arguments are allocated to registers and stack slots as usual in iOS.

The NSRN(*sic*) (?NSAA was intended?) is then rounded up to the next multiple
of 8 bytes, and each variadic argument is assigned to the appropriate number
of 8-byte stack slots.

The C language requires arguments smaller than int to be promoted before a call,
but beyond that, unused bytes on the stack are not specified by the darwinpcs.

(see section 7) As a result of this change, the type va_list is an alias for
char * rather than for the struct type specified in the generic PCS. 
It is also not in the std namespace when compiling C++ code.

#### Stage A

No Changes.

#### Stage B

No changes.

#### Stage C

Insert C.6.5 `D.3`
If the argument is an Integral or Pointer Type, the size of the argument is
less than 4 bytes and the NGRN is less than 8, the argument is sign or zero-
extended as appropriate to 4 bytes.

C.8 Delete rule per `D.4`

C.12
`D.2` Amend to:
The NSAA is rounded up to Natural Alignment of the argument’s type.

C.14 Delete rule per `D.2`

From the observations section:
"Both before and after the layout of each argument, then NSAA will have a
 minimum alignment of 8."  This no longer applies.
 
### AAPCS64 Section 6.

No changes (noting that the `__fp16` type is `IEEE754-2008` format.)

### AAPCS64 Section 7.

7.1 Data Types
7.1.1 Arithmetic Types

Table 3 is amended thus.

| C/C++ Type | Machine Type | Notes |
| --- | --- | --- |
| char | signed byte | compatible with other Darwin variants  |
| wchar_t | int | ditto |

**The size of long double (and by implication the _Imaginary and _Complex C99
variants) is set to be the same as double.**

We need to consider **_very_** carefully how to handle this.  The current
(64bit) long double is mangled as 'e' by the clang toolchain.

7.1.4 Additional Types

Table 5 is amended to reflect the different variadic function rules.

`D.6` The type `va_list` is an alias for `char *`
`D.7` The `va_list` type is _not_ presented in `std::` for C++.

7.2 Argument Passing Conventions

Possibly, might require adjustment for `D.3`?

## PART 2 - Other platform information

### Additional comments from [2]

#### Red Zone

The ARM64 iOS red zone consists of the 128 bytes immediately below the stack
pointer sp. As with the x86-64 ABI, the operating system has committed not to 
modify these bytes during exceptions. User-mode programs can rely on them not
to change unexpectedly, and can potentially make use of the space for local
variables.

In some circumstances, this approach can save an sp-update instruction on
function entry and exit.

At present, it seems, that there's no port using a red zone for AArch64, and
there's no implementation - so this optimisation opportunity will be unused
at least initially.  TODO.

#### Divergences from the Generic C++ ABI

The generic ARM64 C++ ABI is specified in C++ Application Binary Interface
Standard for the ARM 64-bit architecture, which is in turn based on the
Itanium C++ ABI used by many UNIX-like systems.

Some sections are ELF-specific and not applicable to the underlying object
format used by iOS. There are, however, some significant differences from
these specifications in iOS.

##### Name Mangling

When compiling C++ code, types get incorporated into the names of functions
in a process referred to as “mangling.” The iOS ABI differs from the generic
specification in the following small ways.

Because `va_list` is an alias for `char *`, it is mangled in the same way—as
`Pc` instead of `St9__va_list` (Section 7).

NEON vector types are mangled in the same way as their 32-bit ARM counterparts,
rather than using the 64-bit scheme. For example, iOS uses `17__simd128_int32_t`
instead of the generic `11__Int32x4_t`.

##### Other Itanium Divergences

In the generic ABI, empty structs are treated as aggregates with a single byte
member for parameter passing. In iOS, however, they are ignored unless they
have a nontrivial destructor or copy-constructor. If they do have such
functions, they are considered as aggregates with one byte member in the
generic manner.

As with the ARM 32-bit C++ ABI, iOS requires the complete-object (C1) and base-
object (C2) constructors to return this to their callers. Similarly, the
complete object (D1) and base object (D2) destructors return this. This
requirement is not made by the generic ARM64 C++ ABI.

In the generic C++ ABI, array cookies change their size and alignment according
to the type being allocated. As with the 32-bit ARM, iOS provides a fixed
layout of two size_t words, with no extra alignment requirements.

In iOS, object initialization guards are nominally `uint64_t` rather than
`int64_t`.
This affects the prototypes of the functions `__cxa_guard_acquire`,
`__cxa_guard_release` and `__cxa_guard_abort`.

In the generic ARM64 ABI, function pointers whose type differ only in being
extern "C" or extern "C++" are interchangeable. This is not the case in iOS.

### Undocumented items

* The platform ABI contains provisions for the swift language, but since GCC
has no swift FE there's no need to implement them (it might be wise to ensure
that any reserved registers are handled appropriately tho)

* Darwin user-space code is PIC (2) = fPIC (so nominally 'large' but the code
model is not modified by the PIC setting [I think FIXME: check]).

FIXME: ??? I'm not clear about kernel mode at present.

The following symbol kinds always have a GOT indirection for Mach-O-pic.

* undefined external
* weak [not hidden]
* common

FIXME: check other rules for GOT indirections.

### Darwin code models

* TINY is _not_ supported
* SMALL supported (DEFAULT)
* LARGE supported

   AFAICT, Darwin's large model is PIC (with perhaps a very limited number of
   modes)
   However large+PIC is stated to be unimplemented in the current aarch64
   backend so that's a TODO.

FIXME: ??? I'm not clear about kernel mode at present.

### Darwin arm64 TLS

Darwin has a single TLS model (not attempting to implement in the short-
term).  It's closest to ELF xxxxxxx FIXME: which one?

### Generic Darwin/Mach-O Comments for people familiar with ELF.

Darwin
* does _not_ support strong symbol aliases
* does support weak symbol aliases
* supports visibility - default and hidden.
* Has a "for linker only" symbol visibility.

   Such symbols are visible to the static linker (`ld64`), but not externally.
  These are used to support the Mach-O "subsections_by_symbol" linker mode
  (default for > 10years).  Any symbol that is not 'global' and does not begin
  with 'L' (the local symbol designation) is counted as 'linker visible'.
 * does _not_ support 'static' code in the user space
 
   Everything needs to be invoked using the dynamic linker (`dyld`).  There is
  neither crt0.o nor a static edition of libc.

Some versions of Darwin have used 'static' code for kernel modules.
FIXME: ??? what is the kernel model here.

### Darwin Relocations and Assembler syntax

* `Mach-O` for `Arm64` uses a reduced set of relocations c.f. the ELF set.

   There are only 11 entries but the relocation format allows for multiple sizes
(1, 2, 4, 8) where that's appropriate, and for ancillary data (e.g. a scale),
so the actual number of permutations is larger.

* Generally, Darwin supports relocations of the form A - B + signed const

   A must be known (i.e. defined in the current TU).

* `Mach-O` for `Arm64` has postfix assembler syntax.

   Where there's an assembly language representation for the relocation type
   it appears after the name (e.g. `foo@PAGE` in contrast to the ELF
   `:got:foo`).

#### Relocs list

For pointers (no source representation).

`ARM64_RELOC_UNSIGNED = 0`

Must be followed by an `ARM64_RELOC_UNSIGNED`

`ARM64_RELOC_SUBTRACTOR = 1`

A B/BL instruction with 26-bit displacement.
(no source representation)

`ARM64_RELOC_BRANCH26 = 2`

PC-rel distance to page of target [adrp].

`foo@PAGE`

`ARM64_RELOC_PAGE21 = 3`

Offset within page, scaled by r_length [add imm, ld/st].

`foo@PAGEOFF`

`ARM64_RELOC_PAGEOFF12 = 4`

PC-rel distance to page of GOT slot [adrp].

`foo@GOTPAGE`
`ARM64_RELOC_GOT_LOAD_PAGE21 = 5`

Offset within page of GOT slot, scaled by r_length [add imm, ld/st].

`foo@GOTPAGEOFF`

`ARM64_RELOC_GOT_LOAD_PAGEOFF12 = 6`


For pointers to GOT slots.
(4 and 8 byte versions)

`foo@GOT`

`ARM64_RELOC_POINTER_TO_GOT = 7`


PC-rel distance to page of TLVP slot [adrp].

`foo@TVLPPAGE`

`ARM64_RELOC_TLVP_LOAD_PAGE21 = 8`

Offset within page of TLVP slot, scaled by r_length [add imm, ld/st].

`foo@TVLPPAGEOFF`

`ARM64_RELOC_TLVP_LOAD_PAGEOFF12 = 9`

Must be followed by `ARM64_RELOC_PAGE21` or `ARM64_RELOC_PAGEOFF12`.
(no source representation)

`ARM64_RELOC_ADDEND = 10`


## End.
