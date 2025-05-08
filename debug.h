/** Utils of debugging: messages and helper functions */

#ifndef LOCK_DEBUG
#  define LOCK_DEBUG

#  ifdef DEBUG

#    include <utils/mem/mgr.h>

#    include <stdio.h>
#    include <unistd.h>

#    define DB_MSG(msg) \
       fputs(msg "\n", stderr)
#    define DB_BYT(byt) \
       write(STDERR_FILENO, byt, sizeof(byt)/sizeof(*byt) - 1)
#    define DB_NBYT(byt, n) \
       write(STDERR_FILENO, byt, n)
#    define DB_FMT(msg, ...) \
       fprintf(stderr, msg "\n", __VA_ARGS__)

struct mm_header* mm_header_of(void* mem);

#    ifdef DEBUG_MM
void
debug_mm_headers(char* msg, struct mm_header* header);
#    else
#      define debug_mm_headers(...)
#    endif

#  else

#    define DB_MSG(x)
#    define DB_BYT(x)
#    define DB_NBYT(x, y)
#    define DB_FMT(x, ...)

#  endif

#endif
