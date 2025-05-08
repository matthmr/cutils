#include <utils/mem/mgr.h>
#include <utils/macros.h>

#include <string.h>

/** Poor man's `strlen'. `str' needs to be NULL-terminated */
static inline uint cstr_size(char* str) {
  register uint ret = 0;

  while (*str && (ret++, str++));

  return ret;
}

string_i to_string_i(char* str) {
  return (string_i) {
    ._ = str,
    .idx = cstr_size(str)
  };
}

char* from_string_i(string_i stri) {
  // C-str interop
  char* str = mm_alloc(stri.idx + 1);

  memcpy(str, stri._, stri.idx);
  str[stri.idx] = 0x00;

  return str;
}

string_i string_idup(string_i stri) {
  return (string_i) {
    ._ = mm_ndup(stri._, stri.idx),
    .idx = stri.idx,
  };
}

bool string_ieq(string_i stri_a, string_i stri_b) {
  return (stri_a.idx == stri_b.idx &&
          strncmp(stri_a._, stri_b._, stri_a.idx) == 0);
}

////////////////////////////////////////////////////////////////////////////////

bool bf_at(char* bf, uint at) {
  return (bool) bf[(at) >> 3] & (1 << (at & 0x07));
}

void bf_set(char* bf, uint at) {
  bf[(at >> 3)] |= 0xff & (1 << (at & 0x07));
}

void bf_unset(char* bf, uint at) {
  bf[(at >> 3)] &= ~(0xff & (1 << (at & 0x07)));
}

/** Returns the eval with the biggest precendence */
ulong hbit(ulong payload, uint it) {
  // RIGHT-SHIFT and OR at least `it' times
  for (register uint i = 0; i < it; i++) {
    payload |= (payload >> 1);
  }

  // then RIGHT-SHIFT and XOR to get the right-most bit
  return (payload >> 1) ^ payload;
}

/** Internal function for `mmove': move with given bucket size and direction,
    returning the memory at `dest' */
static void*
mmoven(char* src, char* dest, uint src_n, uint oln, bool to_left) {
  uint off = to_left? 0: (src_n - oln);
  bool last = false;

move_n:
  for (uint i = 0; i < oln; i++) {
    dest[i + off] = src[i + off];
  }

  if (last) {
    goto done;
  }

  off = to_left? (off + oln): (off - oln);
  src_n -= oln;

  if (oln > src_n) {
    oln = src_n;
    last = true;

    if (!to_left) {
      off = 0;
    }
  }

  goto move_n;

done:
  return dest;
}

void* mmove(void* src, void* dst, uint src_n) {
  void* ret = src;

  void* dst_bound = (char*)dst + src_n;
  void* src_bound = (char*)src + src_n;

  uint oln = 0;
  bool to_left = true;

  // no overlap: we're good
  if (dst == src) {
    __return();
  }

  // safe to `memcpy': just do it
  if (dst_bound <= src || src_bound <= dst) {
    __return(ret = memcpy(dst, src, src_n));
  }

  // use the difference as the copy size
  if (dst < src) {
    oln = src - dst;
  }
  else {
    oln = dst_bound - src_bound;
    to_left = false;
  }

  ret = mmoven(src, dst, src_n, oln, to_left);

  __defer({
    return ret;
  });
}
