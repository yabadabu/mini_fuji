#pragma once

enum eLogLevel {
  LogError,
  LogWarning,
  LogInfo,
  LogTrace
};

typedef void (*callback_log_fn)( void* context, enum eLogLevel level, const char* msg );
void setLogCallback( void* new_context, enum eLogLevel new_level, callback_log_fn new_callback );
void dbg( enum eLogLevel level, const char* fmt, ...);
