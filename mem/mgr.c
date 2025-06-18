#include "mem/mgr.h"
#include "debug.h"
#include "err.h"

#include "macros.h"

#include <string.h>

// TODO: take the sections with a `header' version of each function
// TODO: use the `FREE' macro to try to free a whole page, so we can *actualy*
//   give memory back to the OS

DEFERR(EOOM, EPAYLOADOF, EDOUBLEFREE, EKUSED) {
  [EOOM] = ERR_MODSTR("mm", "out of memory"),
  [EPAYLOADOF] = ERR_MODSTR("mm", "payload overflow"),
  [EDOUBLEFREE] = ERR_MODSTR("mm", "possible double free"),
  [EKUSED] = ERR_MODSTR("mm", "cannot keep already used memory"),
};

////////////////////////////////////////////////////////////////////////////////

#  ifndef MM_PAGESIZE
// i'm not aware whether you can get the page size at compile time so I hope you
// *do* have 4KiB pages ¯\(ツ)/¯
#    define MM_PAGESIZE (4096)
#  endif

// TODO: implementation with mmap
#ifdef MM_MMAP
#  error "[ !! ] `mmap' implementation is not done"
#else
#  include <unistd.h>
#  define ALLOC(x) sbrk(MM_PAGESIZE*(x))
#  define CBRK()   sbrk(0)
#  define FREE(x) ;
#endif

struct mm_sec* mm_acsec = {0};

////////////////////////////////////////////////////////////////////////////////

// compute the number of pages we need to allocate `size'
#  define PAGES_FOR(size) \
  (((size) / MM_PAGESIZE) + (int) (((size) % MM_PAGESIZE) != 0))
#  define PAYLOAD(p) (p & ~TOP_BIT(p))

// `h' edge predicate
#  define EDGEP(h) TOP_BIT((h)->payload)

// set `h' header as a section edge
#  define SET_EDGE(h) (h)->payload |= (BIT(BITSIZEOF((h)->payload)-1))

// unset `h' header as a section edge
#  define UNSET_EDGE(h) (h)->payload &= PAYLOAD((h)->payload)

#define MM_HEADER_OF(x) (((struct mm_header*) (x)) - 1)
#define MM_PAYLOAD_OF(x) ((byte*)((x) + 1))
#define MM_TSIZE_OF(x) ((x)->payload + sizeof(*(x)))
#define MM_NEXT_WITH(x,n) ((struct mm_header*)((byte*)((x) + 1) + (n)))
#define MM_NEXT(x) MM_NEXT_WITH(x, (x)->payload)

// should *not* be a valid header
#define MM_UPPER(x) (((byte*)(x)) + (x)->cap)
#define MM_BASE(x) ((struct mm_header*) \
                    (((byte*)(x)) + sizeof(struct mm_header) + (x)->m_off))

// predicate for whether this `header' can newly allocate `size'
#define MM_ALLOCP(header, size) \
  (PAYLOAD((header)->payload) >= n && (header)->used == 0)

/** Strategies of allocation */
enum mm_alloc_strat {
  MM_DEFAULT = 0,

  // try to return a header that keeps memory tightly packed together by
  // prioritazing empty headers to the left of us
  MM_CLOSE,

  // try to return a header that has ample room for expansion
  MM_VOLATILE,
};

/** Available header return type */
struct mm_avail_t {
  struct mm_header* header;
  bool on_vm;
};

/** Strat type breaking in `mm_avail_header' */
typedef bool (*mm_avail_header_break) (struct mm_header* prev,
struct mm_header* next, uint n, struct mm_header* ret);

/* Strat type for choosing next header in `mm_avail_header' */
typedef struct mm_header* (*mm_avail_header_next) (struct mm_header* prev,
struct mm_header* next, uint n);

/** Allocates `m_pages' whole pages, returning a pointer to it */
static void* mm_alloc_page(uint pages) {
  __debug_fmt("	-> alloc: allocating %d new page(s)", pages);

  void* ret_t = NULL;

  ret_t = ALLOC(pages);

  if (ret_t == (void*) -1) {
    panic(EOOM);
  }

  return ret_t;
}

static inline void mm_header_manage(struct mm_header* header) {
  mm_acsec = header->sec;
  mm_acsec->cur = header;
}

/** Optional return of next header */
static inline struct mm_header* mm_header_next(struct mm_header* header) {
  if (!header) {
    return NULL;
  }

  register struct mm_sec* sec = header->sec;

  if (EDGEP(header)) {
    return sec->next? MM_BASE(sec->next): NULL;
  }

  register struct mm_header* ret = MM_NEXT(header);

  return (ret >= MM_BASE(sec) && ((byte*)ret < MM_UPPER(sec)))? ret: NULL;
}

/** Optional return of prev header */
static inline struct mm_header* mm_header_prev(struct mm_header* header) {
  if (!header) {
    return NULL;
  }

  return header->prev;
}

// TODO: strat is stub
/** Return an available header to allocate `n' bytes given the strat `like'. In
    the case none exist, return the edge and a flag */
static struct mm_avail_t
mm_header_avail(struct mm_header* header, uint n, enum mm_alloc_strat like) {
  struct mm_avail_t avail = {
    .header = header,
    .on_vm = true,
  };

  bool found = false;

  register uint its_after = 0, its_until = 0;

  struct mm_header* next = header;
  struct mm_header* prev = header;
  struct mm_header* edge = header;

  struct mm_header* ret = NULL;

  // just use this
  if (MM_ALLOCP(header, n)) {
    avail.header = header;

    return avail;
  }

  // iterate through the headers to find one that can support the size of the
  // payload. also, go fowards and back the header chain, and apply the current
  // strat
  for (;;) {
    prev = mm_header_prev(prev);
    next = mm_header_next(next);

    // have a return value, and cannot iterate further: just return it. It's
    // guaranteed to be virtual
    if (found) {
      if (!prev || !its_after) {
done:
        avail.header = ret? ret: edge;

        // there's a rare case where the edge of a section is the only valid
        // header, leaving `ret' unset. If we don't do this, it will allocate
        // another section even though there's plenty of memory left
        avail.on_vm = (bool) (ret || (edge && MM_ALLOCP(edge, n)));
        break;
      }

      its_after--;
    }

    // no more headers at all: just break regardless of strat
    if (!next && !prev) {
      goto done;
    }

    ////

    if (next) {
      edge = next;

      // found a valid header on `next': if not iterating down already, set the
      // limit to 3/4 of the current amount of steps we took to get here, then
      // set the return value to this
      if (!its_after) {
        if (MM_ALLOCP(next, n)) {
          its_after = (its_until * 3) / 4;
          found = true;
          ret = next;
        }

        // we should also set this if we've hit an edge that doesn't fit
        else if (EDGEP(next)) {
          its_after = (its_until * 3) / 4;
          found = true;
        }
      }

      its_until++;
    }

    // found a valid header on `prev': do the same as above
    if (prev && MM_ALLOCP(prev, n)) {
      if (!its_after) {
        its_after = (its_until * 3) / 4;
      }

      found = true;
      ret = prev;
    }
  }

  return avail;
}

/** Partition header in twain, giving `n' bytes to the left side. The header is
    *assumed* to be completly virtualized, otherwise you might mess up the chunk
    chain */
static void mm_header_part(struct mm_header* header, uint n) {
  const register uint payload = PAYLOAD(header->payload);

  // if we can't alloc a new header, just set `vn_header' as ourselves
  struct mm_header* vn_header = header;
  struct mm_header* n_header = NULL;

  register uint rmn = (payload - n);

  // if the remaining memory is less than a header + 1 byte, we just give this
  // memory to the current chunk (aka nothing changes)
  if (rmn > (sizeof(*header))) {
    rmn -= sizeof(*header);

    vn_header = MM_NEXT_WITH(header, n);

    *vn_header = (struct mm_header) {
      .prev = header,
      .sec = header->sec,
      .payload = rmn | EDGEP(header),
      .used = 0,
    };

    header->payload = n;
  }

  if ((n_header = mm_header_next(vn_header))) {
    n_header->prev = vn_header;
  }

  // debug_mm_headers("MM> post-part headers", header);
}

////////////////////////////////////////////////////////////////////////////////

static void mm_init(void) {
  register const int pages_init =
    PAGES_FOR(sizeof(struct mm_header) + sizeof(struct mm_sec));

  void* mem = mm_alloc_page(pages_init);

  mm_acsec = (struct mm_sec*) mem;
  mm_acsec->m_off = 0;

  struct mm_header* header = MM_BASE(mm_acsec);

  mm_acsec->cap = pages_init*MM_PAGESIZE;

  // we init a header containing the whole memory of this section. there should
  // *always* be at least one header in the edge of the section, so that its
  // payload exactly matches the section's edge:
  //   (byte*)header + header->payload == (byte*)header->sec + header->sec->cap
  *header = (struct mm_header) {
    .prev = NULL,
    .payload = (mm_acsec->cap - (sizeof(*header) + sizeof(*mm_acsec))),
    .sec = mm_acsec,
    .used = 0,
  };

  SET_EDGE(header);
  mm_acsec->next = NULL;

  mm_header_manage(header);
}

/** Alloc `n' bytes given we're on an edge header. Returns the allocated header
 */
static struct mm_header* mm_alloc_edge(struct mm_header* header, uint n) {
  const register void* c_brk = CBRK();

  void* mm_upper = MM_UPPER(mm_acsec);
  uint t_size = n + sizeof(*header);

  // it could be the case that some other program used `brk' to allocate some
  // memory: we have to handle it
  if (c_brk != mm_upper) {
    __debug_msg("	-> alloc: allocation clash!");
    t_size += sizeof(*mm_acsec);
  }

  const register uint pages = PAGES_FOR(t_size);

  void* npage = mm_alloc_page(pages);

  // disjoint allocation: new section
  if (npage != mm_upper) {
    SET_EDGE(header);

    mm_acsec = mm_acsec->next = npage;
    struct mm_header* npage_header = (struct mm_header*)
      (((struct mm_sec*) npage) + 1);

    *(struct mm_sec*)mm_acsec = (struct mm_sec) {
      .cap  = pages*MM_PAGESIZE,
      .next = NULL,
      .cur  = npage_header,
    };

    const register int npayload =
      mm_acsec->cap - (sizeof(*header->sec) + sizeof(*header));

    *(struct mm_header*)npage_header = (struct mm_header) {
      .prev = header,
      .sec = mm_acsec,
      .payload = npayload,
      .used = 0,
    };

    SET_EDGE(npage_header);
    header = npage_header;

    mm_header_part(header, n);
  }

  // joined allocation: increase the capacity of the current active section
  else {
    if (header->used) {
      struct mm_header* vn_header = MM_NEXT(header);

      // just in case...
      UNSET_EDGE(header);

      *vn_header = (struct mm_header) {
        .prev = header,
        .sec = header->sec,
        .payload = pages*MM_PAGESIZE - sizeof(*header),
        .used = 0,
      };

      SET_EDGE(vn_header);
      mm_header_part(vn_header, n);

      header = vn_header;
    }

    else {
      header->payload += pages*MM_PAGESIZE;

      mm_header_part(header, n);
    }

    mm_acsec->cap += pages*MM_PAGESIZE;
  }

  return header;
}

/** Internal `mm_alloc' function that takes an alloc strat */
static void* mm_alloc_mem(uint n, enum mm_alloc_strat like) {
  register void* ret = NULL;

  __debug_fmt("MM> alloc: size = %d", n);

  if (!mm_acsec) {
    mm_init();
  }

  struct mm_header* header = mm_acsec->cur;

  struct mm_avail_t avail = mm_header_avail(header, n, like);

  header = avail.header;

  if (avail.on_vm) {
    __debug_msg("	-> alloc: virtual allocation");
    mm_header_part(header, n);
  }
  else {
    __debug_msg("	-> alloc: edge allocation");
    header = mm_alloc_edge(header, n);
  }

  header->used = 1;

  mm_header_manage(header);

  ret = MM_PAYLOAD_OF(header);

  __debug_fmt("	-> alloc: yielding %p at %p", ret, header);
  __debug_mm_headers("MM> post-alloc headers", header);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

void* mm_alloc(uint n) {
  // the default for `mm_alloc' calls is to keep the memory close
  return mm_alloc_mem(n, MM_CLOSE);
}

void* mm_free(void* mem) {
  if (!mem) {
    return mem;
  }

  struct mm_header* header = MM_HEADER_OF(mem);

  __debug_fmt("MM> free %p at %p (%d)", mem, header, header->payload);

  if (!header->used) {
    panic(EDOUBLEFREE);
  }

  // keep if we're still using it
  if ((--header->used)) {
    __debug_fmt("	-> free: kept with %d owners remaining", header->used);

    return mem;
  }

  mem = NULL;

  ////

  struct mm_header* p_header = mm_header_prev(header);

  struct mm_sec* sec = p_header? p_header->sec: header->sec;
  const uint m_off = sec->m_off;

  // fix up for `m_off'
  if ((!p_header || EDGEP(p_header)) && m_off) {
    sec->m_off = 0;

    header = mmove(header, (byte*)header - m_off, sizeof(*header));

    header->payload += m_off;
  }

  if (p_header) {
    if (!p_header->used) {
      __debug_msg("	-> free: prev virtual");

      if (!EDGEP(p_header)) {
        p_header->payload += MM_TSIZE_OF(header) | EDGEP(header);

        header = p_header;
      }
    }

    else {
      __debug_msg("	-> free: prev alloc");
    }
  }

  ////

  struct mm_header* n_header = NULL;

  if ((n_header = mm_header_next(header))) {
    n_header->prev = header;

    if (!n_header->used) {
      __debug_msg("	-> free: next virtual");

      if (!EDGEP(header)) {
        header->payload += MM_TSIZE_OF(n_header) | EDGEP(n_header);

        // could've be changed for the operation above (the one with `p_header')
        if ((n_header = mm_header_next(n_header))) {
          n_header->prev = header;
        }
      }
    }

    else {
      __debug_msg("	-> free: next alloc");
    }
  }

  mm_header_manage(header);

  __debug_mm_headers("MM> post-free headers", header);
  return mem;
}

////////////////////////////////////////////////////////////////////////////////

void* mm_use(void* mem) {
  struct mm_header* header = MM_HEADER_OF(mem);
  header->used++;

  return mem;
}

void* mm_freek(void* mem, uint off, uint n) {
  struct mm_header* m_header = MM_HEADER_OF(mem + off);
  struct mm_header* c_header = MM_HEADER_OF(mem);
  struct mm_header* p_header = mm_header_prev(c_header);

  struct mm_sec* c_sec = c_header->sec;

  register const uint c_payload = c_header->payload;
  register const uint p_payload = p_header? p_header->payload: 0;

  // this crosses the payload boundary. how did this even happen?
  if (off + n > c_payload) {
    panic(EPAYLOADOF);
  }

  // we cannot keep the memory if we can't free it on the first place: just
  // panic
  if (mm_free(mem)) {
    panic(EKUSED);
  }

  // the memory we want to return
  mem = ((byte*) mem) + off;

  // `p_header' is being used, or it doesn't exist: try and create a virtual
  // header with the memory we have in the offset
  if (!p_header || p_header->payload == p_payload) {
    // the offset is too small to create a virtual chunk: this will create some
    // small section of unsable memory near left edges. don't worry, this will
    // be cleared by `mm_free'
    if (off <= sizeof(*c_header)) {
      // try and prevent the above if we have a header right before us
      if (p_header && !EDGEP(p_header)) {
        p_header->payload += off;
      }
      else {
        c_sec->m_off = off;
      }

      m_header = mmove(c_header, m_header, sizeof(*c_header));

      m_header->prev = p_header;
      m_header->payload -= off;
    }

    // the offset is just enough to create a heading memory header
    else {
      mm_header_part(c_header, off - sizeof(*c_header));
    }
  }

  // `p_header' merged with us: we can just give the heading memory to them
  else {
    mm_header_part(p_header, p_payload + off);
  }

  // virtualize the trailing memory
  mm_header_part(m_header, n);

  __debug_fmt("MM> freek: alloc: size = %d", n);
  __debug_fmt("	-> freek: kept %p at %p", MM_PAYLOAD_OF(m_header), m_header);

  mm_use(mem);
  mm_header_manage(m_header);

  return mem;
}

void* mm_expand(void* mem, int n) {
  register void* ret = mem;

  if (!mem) {
    __return(ret = mm_alloc(n));
  }

  struct mm_header* c_header = MM_HEADER_OF(mem);
  struct mm_header* p_header = c_header->prev? c_header->prev: c_header;

  const uint c_used = c_header->used;
  const uint c_payload = c_header->payload;
  const uint payload = c_payload + n;

  /*void* free =*/ (void) mm_free(mem);

  // TODO: without `MM_VOLATILE' implementation, not using `mm_alloc' is dumb
  struct mm_avail_t avail = mm_header_avail(p_header, payload, MM_VOLATILE);
  struct mm_header* header = avail.header;

  if (!avail.on_vm) {
    header = mm_alloc_edge(header, n);
  }

  // `!avail.on_vm' implies this
  if (header != c_header) {
    ret = mmove(mem, MM_PAYLOAD_OF(header), c_payload);
  }

  // clamp the output
  mm_header_part(header, payload);
  mm_header_manage(header);

  header->used = c_used;

  __debug_fmt("MM> alloc: size = %d", header->payload);
  __debug_fmt("	-> alloc: expand from %p", c_header);
  __debug_fmt("	-> alloc: yielding %p at %p",
              MM_PAYLOAD_OF(header), header);

  __defer_for(ret);
}

void* mm_append(void* src, uint n, void* dst) {
  struct mm_header* d_header = dst? MM_HEADER_OF(dst): NULL;
  uint d_payload = d_header? d_header->payload: 0;

  void* ndst = mm_expand(dst, n);
  void* nsrc = ndst + d_payload;

  mmove(src, nsrc, n);

  return ndst;
}

void* mm_alloce(uint n) {
  return mm_alloc_mem(n, MM_VOLATILE);
}

void* mm_ndup(void* src, uint n) {
  void* ret = mm_alloc(n);

  ret = memcpy(ret, src, n);

  return ret;
}

void* mm_allocz(uint n) {
  void* ret = mm_alloc(n);

  explicit_bzero(ret, n);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

void mm_manage(void* mem) {
  mm_header_manage(MM_HEADER_OF(mem));
}

void* mm_managed(void) {
  return mm_acsec? MM_PAYLOAD_OF(mm_acsec->cur): NULL;
}
