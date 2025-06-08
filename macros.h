/** Utils for *generic* macros; should not be dependent on any other util */

#ifndef LOCK_MACROS
#  define LOCK_MACROS

#  define BITSIZEOF(x) sizeof(x)*8

#  define LINE(x) \
  x "\n"

// machine-human
#  define IDX_MH(x) \
  ((x) + 1)

// human-machine
#  define IDX_HM(x) \
  ((x) - 1)

#  define MAYBE_INIT(x) \
  __assert((x) == 0, ERR());

#  define ITOA(x) ((x) + 0x30)
#  define ATOI(x) ((x) - 0x30)

////
/** ERROR CONTROL MACROS

    These macros are used for error control. There are three main categories:

    - defer macros
    - return macros
    - assert macros

    You have to define a defer macro in the end of your function.
    Unconditionally jump to them using defer macros, or conditionally with
    assert macros. If using the default defer or assert family of functions,
    make sure to define a stack variable named `ret'

    Every macro has modifiers which are separated by underscores. The positional
    argument is the modifier + 1. E.g:

    __return_as_with(x,y)
      - return
        - as x
        - with (the value of) y */

///

// defer guard
#  define __defer(...) \
  __defer: \
    __VA_ARGS__

// generic defer guard, returning for `x'
#  define __defer_for(x) \
  __defer: \
    return (x)

// defer guard, returning for `x', executing `y'
#  define __defer_for_with(x, y) \
  __defer: \
    y; \
    return (x)

///

// return statement, executing `x'
#  define __return(...) \
  __VA_ARGS__; \
  goto __defer

// return statement, with the value of `x'
#  define __return_as(x) \
  ret = (x); \
  __return()

// return statement, with the value of `y' for `x'
#  define __return_for_as(x,y) \
  (x) = (y); \
  __return()

// return statement, with the value of `x', executing `y'
#  define __return_as_with(x,y) \
  ret = (x); \
  (y); \
  __return()

// return statement, with the value of `y', for `x', executing `z'
#  define __return_for_as_with(x,y,z) \
  (x) = (y); \
  (z); \
  __return()

///

// conditional return statement
#  define __return_if(x) \
  if (x) { \
    __return(); \
  }

// conditional return statement, with return set to `y'
#  define __return_if_as(x,y) \
  if (x) { \
    __return_as(y); \
  }

// conditional return statement, executing `y'
#  define __return_if_with(x,y) \
  if (x) { \
    y; \
    __return(); \
  }

// conditional return statement, with return set to `y', executing `z'
#  define __return_if_as_with(x,y,z) \
  if (x) { \
    __return_as_with((y), (z)); \
  }

// conditional return statement, with the value of `z' set to `y'
#  define __return_if_for_as(x,y,z) \
  if (x) { \
    __return_for_as((y), (z)); \
  }

// conditional return statement, with the value of `z' set to `y', executing `w'
#  define __return_if_for_as_with(x,y,z,w) \
  if (x) { \
    __return_for_as_with((y), (z), (w)); \
  }

///

// generic assert statement
#  define __assert(x,y) \
  __return_if_as(!(x), (y))

// assert statement with the value of `z'
#  define __assert_for(x,y,z) \
  __return_if_for_as(!(x), (y), (z))

// assert statement executing `z'
#  define __assert_with(x,y,z) \
  __return_if_as_with(!(x), (y), (z))

// assert statement with the value of `z', executing `w'
#  define __assert_for_with(x,y,z,w) \
  __return_if_for_as_with(!(x), (y), (z), (w))

///

// generic 'error control' header: use this at the beginning of a function to
// declare common errctl variables
#  define __with_errctl(x) \
  register int ret = 0

// `with_errctl' with another variable name
#  define __with_errctl_as(x) \
  register int x = 0

// generic 'error control' footer: use this at the end of a function to
// exit with common errctl variables
#  define __defer_errctl(x) \
  __defer_for(ret)

///

#endif
