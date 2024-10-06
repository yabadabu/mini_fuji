#include "log.h"
#include <stdio.h>
#include <stdarg.h>

typedef struct {
   void*           context;
   enum eLogLevel  level;
   callback_log_fn callback;
} callback_log_t;

static callback_log_t callback_log = (callback_log_t){ .context = NULL, .level = LogError, .callback = NULL };

void setLogCallback( void* new_context, enum eLogLevel new_level, callback_log_fn new_callback ) {
  callback_log.context = new_context;
  callback_log.level = new_level;
  callback_log.callback = new_callback;
}

void dbg(enum eLogLevel level, const char* fmt, ...) {
  if (!callback_log.callback || callback_log.level < level)
    return;
  char buf[2048];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
  if (n < 0)
    buf[sizeof(buf) - 1] = 0x00;
  va_end(ap);
  callback_log.callback(callback_log.context, level, buf);
}

