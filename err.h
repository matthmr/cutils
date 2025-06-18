/** Utils for erroring */

#ifndef LOCK_ERR
#  define LOCK_ERR

#  include "mem/dat.h"
#  include "macros.h"

#  define STRING_S_NULL STRING_S("")

#  define ERR_STR(msg) \
  STRING_S(LINE("[ !! ] " msg))

#  define ERR_MODSTR(mod, msg) \
  STRING_S(LINE("[ !! ] " mod ": " msg))

// bubble up err with previous error code
#  define ERR(x) \
  eerr(STRING_S_NULL, 0, false)

// bubble up panic with previous error code
#  define PANIC(x) \
  epanic(STRING_S_NULL, 0)

// module-local errors
#  define DEFERR(...) \
  enum ecode {EOK = 0, __VA_ARGS__}; \
  static const string_s emsg[] =

// error/panic functions to be used with module-local errors
#  define err(ecode) eerr(emsg[ecode], ecode, false)
#  define ferr(ecode) eerr(emsg[ecode], ecode, true)
#  define panic(ecode) epanic(emsg[ecode], ecode)

// error/panic functions to be used with generic errors
#  define errm(emsg, ecode) eerr(emsg, ecode, false)
#  define ferrm(emsg, ecode) eerr(emsg, ecode, true)
#  define panicm(emsg, ecode) epanic(emsg, ecode)

#  define ERR_BUBBLE_UP (-1)

/** Top-level error function. Handlable */
int eerr(const string_s emsg, int code, bool force_msg);

/** Top-level panic function. Unhandlable */
int epanic(const string_s emsg, int code);

/** Reset error trigger */
void ereset(void);

/** Get the code that triggered the current error */
int eget(void);

#endif
