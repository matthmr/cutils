#include <utils/debug.h>
#include <utils/err.h>

#include <stdlib.h>
#include <unistd.h>

static int tecode = 0;
static bool did_msg = false;

static int panic_enopreverr(void) {
  return panicm(
    ERR_MODSTR("err", "bubbling up error/panic with no previous error"), -1);
}

int eerr(const string_s emsg, int ecode, bool force_msg) {
  if (!did_msg || force_msg) {
    if (!emsg._) {
      return panic_enopreverr();
    }

    did_msg = true;
    tecode = ecode;

    DB_FMT("ERR> error (%d), with message:", ecode);
    write(STDERR_FILENO, emsg._, emsg.sz);

    return ecode;
  }

  DB_FMT("	-> err: bubbling up from error (%d)", tecode);
  return ERR_BUBBLE_UP;
}

void ereset(void) {
  tecode = 0;
  did_msg = false;
}

int eget(void) {
  return tecode;
}

int epanic(const string_s emsg, int ecode) {
  if (!emsg._) {
    return panic_enopreverr();
  }

  write(STDERR_FILENO, emsg._, emsg.sz);

  exit(ecode);

  return -1;
}
