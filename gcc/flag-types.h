/* Compilation switch flag type definitions for GCC.
   Copyright (C) 1987-2014 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_FLAG_TYPES_H
#define GCC_FLAG_TYPES_H

enum debug_info_type
{
  NO_DEBUG,	    /* Write no debug info.  */
  DBX_DEBUG,	    /* Write BSD .stabs for DBX (using dbxout.c).  */
  SDB_DEBUG,	    /* Write COFF for (old) SDB (using sdbout.c).  */
  DWARF2_DEBUG,	    /* Write Dwarf v2 debug info (using dwarf2out.c).  */
  XCOFF_DEBUG,	    /* Write IBM/Xcoff debug info (using dbxout.c).  */
  VMS_DEBUG,        /* Write VMS debug info (using vmsdbgout.c).  */
  VMS_AND_DWARF2_DEBUG /* Write VMS debug info (using vmsdbgout.c).
                          and DWARF v2 debug info (using dwarf2out.c).  */
};

enum debug_info_levels
{
  DINFO_LEVEL_NONE,	/* Write no debugging info.  */
  DINFO_LEVEL_TERSE,	/* Write minimal info to support tracebacks only.  */
  DINFO_LEVEL_NORMAL,	/* Write info for all declarations (and line table).  */
  DINFO_LEVEL_VERBOSE	/* Write normal info plus #define/#undef info.  */
};

/* A major contribution to object and executable size is debug
   information size.  A major contribution to debug information
   size is struct descriptions replicated in several object files.
   The following function determines whether or not debug information
   should be generated for a given struct.  The indirect parameter
   indicates that the struct is being handled indirectly, via
   a pointer.  See opts.c for the implementation. */

enum debug_info_usage
{
  DINFO_USAGE_DFN,	/* A struct definition. */
  DINFO_USAGE_DIR_USE,	/* A direct use, such as the type of a variable. */
  DINFO_USAGE_IND_USE,	/* An indirect use, such as through a pointer. */
  DINFO_USAGE_NUM_ENUMS	/* The number of enumerators. */
};

/* A major contribution to object and executable size is debug
   information size.  A major contribution to debug information size
   is struct descriptions replicated in several object files. The
   following flags attempt to reduce this information.  The basic
   idea is to not emit struct debugging information in the current
   compilation unit when that information will be generated by
   another compilation unit.

   Debug information for a struct defined in the current source
   file should be generated in the object file.  Likewise the
   debug information for a struct defined in a header should be
   generated in the object file of the corresponding source file.
   Both of these case are handled when the base name of the file of
   the struct definition matches the base name of the source file
   of the current compilation unit.  This matching emits minimal
   struct debugging information.

   The base file name matching rule above will fail to emit debug
   information for structs defined in system headers.  So a second
   category of files includes system headers in addition to files
   with matching bases.

   The remaining types of files are library headers and application
   headers.  We cannot currently distinguish these two types.  */

enum debug_struct_file
{
  DINFO_STRUCT_FILE_NONE,   /* Debug no structs. */
  DINFO_STRUCT_FILE_BASE,   /* Debug structs defined in files with the
                               same base name as the compilation unit. */
  DINFO_STRUCT_FILE_SYS,    /* Also debug structs defined in system
                               header files.  */
  DINFO_STRUCT_FILE_ANY     /* Debug structs defined in all files. */
};

/* Enumerate visibility settings.  This is deliberately ordered from most
   to least visibility.  */
#ifndef SYMBOL_VISIBILITY_DEFINED
#define SYMBOL_VISIBILITY_DEFINED
enum symbol_visibility
{
  VISIBILITY_DEFAULT,
  VISIBILITY_PROTECTED,
  VISIBILITY_HIDDEN,
  VISIBILITY_INTERNAL
};
#endif

/* Enumerate Objective-c instance variable visibility settings. */

enum ivar_visibility
{
  IVAR_VISIBILITY_PRIVATE,
  IVAR_VISIBILITY_PROTECTED,
  IVAR_VISIBILITY_PUBLIC,
  IVAR_VISIBILITY_PACKAGE
};

/* The stack reuse level.  */
enum stack_reuse_level
{
  SR_NONE,
  SR_NAMED_VARS,
  SR_ALL
};

/* The algorithm used for the integrated register allocator (IRA).  */
enum ira_algorithm
{
  IRA_ALGORITHM_CB,
  IRA_ALGORITHM_PRIORITY
};

/* The regions used for the integrated register allocator (IRA).  */
enum ira_region
{
  IRA_REGION_ONE,
  IRA_REGION_ALL,
  IRA_REGION_MIXED,
  /* This value means that there were no options -fira-region on the
     command line and that we should choose a value depending on the
     used -O option.  */
  IRA_REGION_AUTODETECT
};

/* The options for excess precision.  */
enum excess_precision
{
  EXCESS_PRECISION_DEFAULT,
  EXCESS_PRECISION_FAST,
  EXCESS_PRECISION_STANDARD
};

/* Type of stack check.  */
enum stack_check_type
{
  /* Do not check the stack.  */
  NO_STACK_CHECK = 0,

  /* Check the stack generically, i.e. assume no specific support
     from the target configuration files.  */
  GENERIC_STACK_CHECK,

  /* Check the stack and rely on the target configuration files to
     check the static frame of functions, i.e. use the generic
     mechanism only for dynamic stack allocations.  */
  STATIC_BUILTIN_STACK_CHECK,

  /* Check the stack and entirely rely on the target configuration
     files, i.e. do not use the generic mechanism at all.  */
  FULL_BUILTIN_STACK_CHECK
};

/* Names for the different levels of -Wstrict-overflow=N.  The numeric
   values here correspond to N.  */

enum warn_strict_overflow_code
{
  /* Overflow warning that should be issued with -Wall: a questionable
     construct that is easy to avoid even when using macros.  Example:
     folding (x + CONSTANT > x) to 1.  */
  WARN_STRICT_OVERFLOW_ALL = 1,
  /* Overflow warning about folding a comparison to a constant because
     of undefined signed overflow, other than cases covered by
     WARN_STRICT_OVERFLOW_ALL.  Example: folding (abs (x) >= 0) to 1
     (this is false when x == INT_MIN).  */
  WARN_STRICT_OVERFLOW_CONDITIONAL = 2,
  /* Overflow warning about changes to comparisons other than folding
     them to a constant.  Example: folding (x + 1 > 1) to (x > 0).  */
  WARN_STRICT_OVERFLOW_COMPARISON = 3,
  /* Overflow warnings not covered by the above cases.  Example:
     folding ((x * 10) / 5) to (x * 2).  */
  WARN_STRICT_OVERFLOW_MISC = 4,
  /* Overflow warnings about reducing magnitude of constants in
     comparison.  Example: folding (x + 2 > y) to (x + 1 >= y).  */
  WARN_STRICT_OVERFLOW_MAGNITUDE = 5
};

/* Floating-point contraction mode.  */
enum fp_contract_mode {
  FP_CONTRACT_OFF = 0,
  FP_CONTRACT_ON = 1,
  FP_CONTRACT_FAST = 2
};

/* Vectorizer cost-model.  */
enum vect_cost_model {
  VECT_COST_MODEL_UNLIMITED = 0,
  VECT_COST_MODEL_CHEAP = 1,
  VECT_COST_MODEL_DYNAMIC = 2,
  VECT_COST_MODEL_DEFAULT = 3
};


/* Different instrumentation modes.  */
enum sanitize_code {
  /* AddressSanitizer.  */
  SANITIZE_ADDRESS = 1 << 0,
  SANITIZE_USER_ADDRESS = 1 << 1,
  SANITIZE_KERNEL_ADDRESS = 1 << 2,
  /* ThreadSanitizer.  */
  SANITIZE_THREAD = 1 << 3,
  /* LeakSanitizer.  */
  SANITIZE_LEAK = 1 << 4,
  /* UndefinedBehaviorSanitizer.  */
  SANITIZE_SHIFT = 1 << 5,
  SANITIZE_DIVIDE = 1 << 6,
  SANITIZE_UNREACHABLE = 1 << 7,
  SANITIZE_VLA = 1 << 8,
  SANITIZE_NULL = 1 << 9,
  SANITIZE_RETURN = 1 << 10,
  SANITIZE_SI_OVERFLOW = 1 << 11,
  SANITIZE_BOOL = 1 << 12,
  SANITIZE_ENUM = 1 << 13,
  SANITIZE_FLOAT_DIVIDE = 1 << 14,
  SANITIZE_FLOAT_CAST = 1 << 15,
  SANITIZE_BOUNDS = 1 << 16,
  SANITIZE_ALIGNMENT = 1 << 17,
  SANITIZE_UNDEFINED = SANITIZE_SHIFT | SANITIZE_DIVIDE | SANITIZE_UNREACHABLE
		       | SANITIZE_VLA | SANITIZE_NULL | SANITIZE_RETURN
		       | SANITIZE_SI_OVERFLOW | SANITIZE_BOOL | SANITIZE_ENUM
		       | SANITIZE_BOUNDS | SANITIZE_ALIGNMENT,
  SANITIZE_NONDEFAULT = SANITIZE_FLOAT_DIVIDE | SANITIZE_FLOAT_CAST
};

/* flag_vtable_verify initialization levels. */
enum vtv_priority {
  VTV_NO_PRIORITY       = 0,  /* i.E. Do NOT do vtable verification. */
  VTV_STANDARD_PRIORITY = 1,
  VTV_PREINIT_PRIORITY  = 2
};

/* flag_lto_partition initialization values.  */
enum lto_partition_model {
  LTO_PARTITION_NONE = 0,
  LTO_PARTITION_ONE = 1,
  LTO_PARTITION_BALANCED = 2,
  LTO_PARTITION_1TO1 = 3,
  LTO_PARTITION_MAX = 4
};

/* The code generator used by graphite */
enum fgraphite_generator {
  FGRAPHITE_CODE_GEN_ISL = 0,
  FGRAPHITE_CODE_GEN_CLOOG = 1
};

#endif /* ! GCC_FLAG_TYPES_H */
