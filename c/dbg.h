#pragma once

enum eDbgLevel {
  DbgError,
  DbgWarning,
  DbgInfo,
  DbgTrace
};

typedef void (*callback_log_fn)( void* context, enum eDbgLevel level, const char* msg );
void setDbgCallback( void* new_context, enum eDbgLevel new_level, callback_log_fn new_callback );
void dbg( enum eDbgLevel level, const char* fmt, ...);
