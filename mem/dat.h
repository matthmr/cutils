/** Utils for generic memory; strings, sized strings, bit-fields, move etc */

// TODO: we can break the string side of this

#ifndef LOCK_DAT
#  define LOCK_DAT

//// ALIASES

#  ifndef false
#    define false 0x0
#  endif

#  ifndef true
#    define true  0x1
#  endif

#  ifndef NULL
#    define NULL ((void*)0)
#  endif

// set `n'th bit to 1
#  define BIT(n) (1 << (n))

// single out the top bit of `x' integer type
#  define TOP_BIT(x) ((x) & (1 << (sizeof(x)*8 - 1)))

typedef unsigned char* mem;
typedef unsigned char bool, byte, uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

//// BIT-FIELD UTILS

/** Get boolean bit value of bit at position `at' given bit-field `bf' */
bool bf_at(char* bf, uint at);

/** Set boolean bit value of bit at position `at' given bit-field `bf' to true
   */
void bf_set(char* bf, uint at);

/** Set boolean bit value of bit at position `at' given bit-field `bf' to false
   */
void bf_unset(char* bf, uint at);

/** Move memory from `src' to `dst', given their respective sizes (`src_n'),
    taking care of overlapping, returning the memory at `dst' */
void* mmove(void* src, void* dst, uint src_n);

/** Return the highest-set bit from `payload', iterating at most `it' times */
ulong hbit(ulong payload, uint it);

/** Reverse bits from `payload' of `size' */
// ulong rbit(ulong payload, uint size);

//// GENERIC DATA UTILS

#  define SIZEOF(x) (sizeof((x))/sizeof(*(x)))

/** Generic sized data, alloc-safe. Similar to `string*' */
struct dat_sz {
  uint sz;
  void* _;
};

/** Generic typed data */
struct dat_t {
  uint typ;
  void* _;
};

/** Generic sized and typed data, alloc safe */
struct dat_szt {
  uint sz;
  uint typ;
  void* _;
};

/** `struct dat_sz' type */
typedef struct dat_sz dat_sz;

/** `struct dat_t' type */
typedef struct dat_t dat_t;

/** `struct dat_szt' type */
typedef struct dat_szt dat_szt;

// `NULL' value for `dat_*'
#  define DAT_NULL {0}

// convert raw data `x' into generic `dat_szt' given type  `t'
#  define DATSZT(x, t) (dat_szt) {._ = x, .sz = 0, .typ = (t)}

// convert `dat_szt' into genric `dat_sz'
#  define DATSZT_SZ(x) (dat_sz) {._ = (x)._, .sz = (x).sz}

// convert `dat_sz' `x' into generic `dat_szt' given type  `t'
#  define DATSZT_ST(x, t) (dat_szt) {._ = (x)._, .sz = (x).sz, .typ = (t)}

// convert raw data `x' into generic `dat_sz'
#  define DATSZ(x) (dat_sz) {._ = (x), .sz = (sizeof(x)/sizeof(*(x)))-1}

// copy `src' to `mem' (`mem' usually being stack-local, from `src' usually
// being of `*_sz*' types)
#  define mcpy(src,mem) (void) mmove((src), (mem), (sizeof(*(mem))))

//// LIST UTILS

/** Generic linked list, alloc safe */
struct dat_ll {
  struct dat_ll* next;
  void* elem;
};

/** Generic linked list, with sized data as the element. Alloc safe */
struct dat_llsz {
  struct dat_llsz* next;
  dat_sz elem;
};

/** Generic linked list, with sized and typed data as the element. Alloc safe */
struct dat_llszt {
  struct dat_llszt* next;
  dat_szt elem;
};

/** Generic linked list, with typed data as the element. Alloc safe */
struct dat_llt {
  struct dat_llt* next;
  dat_t elem;
};

/** `struct dat_ll' type */
typedef struct dat_ll dat_ll;

/** `struct dat_llt' type */
typedef struct dat_llt dat_llt;

/** `struct dat_llsz' type */
typedef struct dat_llsz dat_llsz;

/** `struct dat_llszt' type */
typedef struct dat_llszt dat_llszt;

// pass list `lst' (as a pointer) as "%self, %size"
#  define LSTSZ(lst) (lst), SIZEOF(lst)

//// STRING UTILS

/** Sized string */
struct string_s {
  const char* _;
  const uint sz;
};

#  define STRING_S(x) \
  (struct string_s) {._ = (x), .sz = (SIZEOF(x) - 1)}

/** Incremental string. Usage may mask `string_s' for non-constant strings;
    basically an incremental sized string */
struct string_i {
  char* _;
  uint idx;
};

// simply reset the incremental string. does not free memory (obviously)
#  define INC_RESET(x) ((x).idx = 0)

typedef struct string_s string_s;
typedef struct string_i string_i;

/** Boolean return of two `string_i's equality (overflow-safe) */
bool string_ieq(string_i stri_a, string_i stri_b);

/** Duplicate `string_i' `stri' */
string_i string_idup(string_i stri);

/** Convert from Cstr to `string_i' */
string_i to_string_i(char* str);

/** Convert from `string_i' to Cstr */
char* from_string_i(string_i stri);

#endif
