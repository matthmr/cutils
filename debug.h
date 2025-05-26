/** Utils of debugging: messages and helper functions */

#ifndef LOCK_DEBUG
#  define LOCK_DEBUG

#  define __debug_mm_headers(...)

#  ifdef DEBUG

#    include <utils/mem/mgr.h>

#    include <stdio.h>
#    include <unistd.h>

#    define __debug(...) \
     __VA_ARGS__
#    define __debug_msg(msg) \
       fputs(msg "\n", stderr)
#    define __debug_byt(byt) \
       write(STDERR_FILENO, byt, sizeof(byt)/sizeof(*byt) - 1)
#    define __debug_bytsz(byt, n) \
       write(STDERR_FILENO, byt, n)
#    define __debug_fmt(msg, ...) \
       fprintf(stderr, msg "\n", __VA_ARGS__)

struct mm_header* mm_header_of(void* mem);

#    ifdef DEBUG_MM
#      undef __debug_mm_headers
void
__debug_mm_headers(char* msg, struct mm_header* header);
#    endif

#  else
#    define __debug(...)
#    define __debug_msg(x)
#    define __debug_byt(x)
#    define __debug_bytsz(x, y)
#    define __debug_fmt(x, ...)
#  endif

#endif
