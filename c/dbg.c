#include "dbg.h"
#include <stdio.h>
#include <stdarg.h>

typedef struct {
   void*           context;
   enum eDbgLevel  level;
   callback_dbg_fn callback;
} callback_dbg_t;

static callback_dbg_t callback_dbg = (callback_dbg_t){ .context = NULL, .level = DbgError, .callback = NULL };

void setDbgCallback( void* new_context, enum eDbgLevel new_level, callback_dbg_fn new_callback ) {
  callback_dbg.context = new_context;
  callback_dbg.level = new_level;
  callback_dbg.callback = new_callback;
}

void dbg(enum eDbgLevel level, const char* fmt, ...) {
  if (!callback_dbg.callback || callback_dbg.level < level)
    return;
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
  if (n < 0)
    buf[sizeof(buf) - 1] = 0x00;
  va_end(ap);
  callback_dbg.callback(callback_dbg.context, level, buf);
}

