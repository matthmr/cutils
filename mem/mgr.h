/** Memory manager interface */

#ifndef LOCK_MM
#  define LOCK_MM

#  include <utils/mem/dat.h>

#  define mm_alloc_from(x) mm_alloc(sizeof(*(x)))
#  define mm_allocz_from(x) mm_allocz(sizeof(*(x)))
#  define mm_dup(x) mm_ndup((x), sizeof(*(x)))

#  define MM_ALLOC(x) (x) = mm_alloc_from(x)
#  define MM_ALLOCZ(x) (x) = mm_allocz_from(x)
#  define MM_DUP(x,y) (x) = mm_dup(y)

struct mm_sec;

// TODO: do we need `sec'?
/** Memory chunk header */
struct mm_header {
  /* the previous header */
  struct mm_header* prev;

  /* header section interface */
  struct mm_sec* sec;

  /* size of the current payload. the next header is either the beginning of
     next section, or after `payload' bytes. the sign bit represents if this
     is the last header of its section; i.e. the `edge' */
  int payload;

  /* allocation status for this header. doubles as a built-in reference counter
     that can be changed by `mm_use' and `mm_free'; usage from `mm_alloc' is
     similar to `malloc' */
  uint used;
};

/** Memory allocator section interface. The layout of memory is that the first
    `sizeof(struct mm_header)' bytes are for the header in the chunk, and the
    rest of bytes discriminated in the header are for the payload

    This structure manages a memory section, which is more than or equal to one
    page of memory. A section is any non-consecutive memory segments, which are
    themselves consecutive */
struct mm_sec {
  /* the current memory capacity: the whole thing, including the section
     interface, the payloads and the headers */
  uint cap;

  /* (nternal) valid memory offset. up to sizeof(struct mm_header) */
  uint m_off;

  /* next section */
  struct mm_sec* next;

  /* the current header */
  struct mm_header* cur;
};

////////////////////////////////////////////////////////////////////////////////

/** The active memory section */
extern struct mm_sec* mm_acsec;

/** Allocates `n' bytes, returning a pointer to the memory */
void* mm_alloc(uint n);

/** 'Owns' `mem'. Only valid to call if `mem' is resultant from `mm_alloc'.
    `mm_alloc' also implicitly calls this function, so don't
    use `mm_use(mm_alloc(...))' */
void* mm_use(void* mem);

/** Deallocates the memory of `mem'. Returns `NULL' if the freeing was
    successful, otherwise returns `mem' */
void* mm_free(void* mem);

///

/** Advise the current section to take `mem' as the current header. `mem'
   should be MM-memory */
void mm_manage(void* mem);

/** Return the current advised section */
void* mm_managed(void);

/** Duplicate `n' bytes of `src' */
void* mm_ndup(void* src, uint n);

/** Call `mm_alloc', and zero out the result */
void* mm_allocz(uint n);

/** Call `mm_alloc' with an advice to keep the chunk as big as possible so we
    may expand it later */
void* mm_alloce(uint n);

/** Expand `mem' by `n' bytes (negative means 'contract'), returning the
    resulting memory */
void* mm_expand(void* mem, int n);

/** Append `n' bytes of `src' into `dst', returning the pointer to the beginning
    of the possibly newly allocated memory */
void* mm_append(void* src, uint n, void* dst);

/** Call `mm_free' on `mem', but keep a `n' bytes after `off' bytes of the
    paylod allocated. Returns the memory kept */
void* mm_freek(void* mem, uint off, uint n);

#endif
