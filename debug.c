#include <utils/macros.h>
#include <utils/debug.h>

#ifdef DEBUG_MM

#include <stdio.h>
#include <string.h>

#define MM_NEXT_WITH(x,n) ((struct mm_header*)((byte*)((x) + 1) + (n)))
#define MM_NEXT(x) MM_NEXT_WITH(x, (x)->payload)
#define MM_UPPER(x) (((byte*)(x)) + (x)->cap)
#define MM_BASE(x) ((struct mm_header*) \
                    (((byte*)(x)) + sizeof(struct mm_header) + (x)->m_off))
#  define EDGEP(h) TOP_BIT((h)->payload)

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

void
debug_mm_headers(char* msg, struct mm_header* header) {
  struct mm_sec* sec;

  uint size = 0;
  uint tsize = 0;
  uint headers = 0;
  uint secs = 0;

  if (msg) {
    // puts(msg);
    fprintf(stderr, "%s\n", msg);
  }

  // get the ending sector ...
  for (sec = header->sec; sec->next; sec = sec->next);

  // ... then the ending header ...
  header = (struct mm_header*)(sec + 1);

  // ... go to the edge ...
  for (;;) {
    struct mm_header* n_header = mm_header_next(header);

    if (!n_header) {
      break;
    }

    header = n_header;
  }

  fprintf(stderr, "---\n");

  char stat[6] = {0};
  uint stati = 0;

  // ... then iterate backwards:
  for (;;header = header->prev) {
    bzero(stat, SIZEOF(stat));
    stati = 0;

    struct mm_sec* sec = header->sec;
    struct mm_header* p_header = NULL;

    if (!header->used) {
      stat[stati] = 'f';
      stati++;
    }
    else {
      stat[stati] = 'm';
      stati++;
    }
    if (EDGEP(header)) {
      stat[stati] = 'e';
      stati++;
    }
    if (header == sec->cur) {
      stat[stati] = 'c';
      stati++;
    }
    if (header == mm_acsec->cur) {
      stat[stati] = 'C';
      stati++;
    }
    if (header == (struct mm_header*)
                  ((byte*)sec + sizeof(*header) + sec->m_off)) {
      stat[stati] = 'b';
      stati++;
    }

    const uint payload = header->payload & ~BIT(BITSIZEOF(header->payload)-1);

    fprintf(stderr, "%p : %p : %p [%d%s]\n",
            header->sec, header, header+1, payload, stat);

    if (p_header && mm_header_next(header) != p_header) {
      fprintf(stderr, "[ !! ] CLASH: %p -!> %p\n", header, p_header);
    }

    size += payload;
    headers++;

    p_header = header;

    if (header->prev && header->prev->sec != sec) {
      tsize = size + headers*sizeof(*header) + sizeof(*sec);
      fprintf(stderr, "\nsize(%d): %d (%d), total: %d\n", headers, size,
             tsize, sec->cap);
      fprintf(stderr, "---\n");

      size = 0;
      headers = 0;
    }

    if (!header->prev) {
      tsize = size + headers*sizeof(*header) + sizeof(*sec);
      fprintf(stderr, "\nsize(%d): %d (%d), total: %d\n", headers, size,
             tsize, sec->cap);

      break;
    }
  }

  fprintf(stderr, "---\n");
}
#endif

struct mm_header* mm_header_of(void* mem) {
  return ((struct mm_header*) mem - 1);
}
