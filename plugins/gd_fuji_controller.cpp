#include <cassert>
#include "gd_fuji_controller.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
  #include "actions_list.h"
  #include "dbg.h"
}

using namespace godot;

// ---------------------------------------------------------------------
void GDFujiController::_bind_methods() {
  ClassDB::bind_method(D_METHOD("start"), &GDFujiController::start);
  ClassDB::bind_method(D_METHOD("stop"), &GDFujiController::stop);
  ClassDB::bind_method(D_METHOD("toggle"), &GDFujiController::toggle);
  ClassDB::bind_method(D_METHOD("set_max_time_per_step"), &GDFujiController::set_max_time_per_step);
  ADD_SIGNAL(MethodInfo("camera_event", PropertyInfo(Variant::STRING, "line")));
  ADD_SIGNAL(MethodInfo("camera_log", PropertyInfo(Variant::STRING, "line")));
  ADD_SIGNAL(MethodInfo("download_progress", PropertyInfo(Variant::INT, "bytes_downloaded"), PropertyInfo(Variant::INT, "bytes_required")));
}

void GDFujiController::set_max_time_per_step( int new_max_time_per_step_usecs ) {
  ev.max_time_per_step = new_max_time_per_step_usecs;
}

static void on_event(void* context, const char* msg ) {
  assert( context );
  GDFujiController* c = (GDFujiController*) context;
  String str( msg );
  UtilityFunctions::print( str );
  c->emit_signal( "camera_event", str );
}

static void on_download_progress(void* context, uint32_t curr, uint32_t required ) {
  assert( context );
  GDFujiController* c = (GDFujiController*) context;
  c->emit_signal( "download_progress", curr, required );
}

static void on_dbg_msg( void* context, enum eDbgLevel level, const char* msg ) {
  assert( context );
  // For the UI
  GDFujiController* c = (GDFujiController*) context;
  String str( msg );
  c->emit_signal( "camera_log", str );

  // For the console
  int nchars = strlen( msg );
  if( nchars > 0 && msg[ nchars - 1 ] )
    ((char*) msg)[ nchars - 1 ] = 0x00;
  UtilityFunctions::print( msg );
}

// ---------------
GDFujiController::GDFujiController() {
  setDbgCallback( this, DbgTrace, &on_dbg_msg );
  is_valid = conn_create( &conn );
  conn.on_event = (callback_event_t) { .context = this, .callback = &on_event };
  conn.on_progress = (callback_progress_t){ .context = this, .callback = &on_download_progress, .enabled = false };
  eval_create( &ev, &conn, actions_take );
  ev.max_time_per_step = 1000;
}

GDFujiController::~GDFujiController() {
  if( is_valid )
    conn_destroy( &conn );
}

bool GDFujiController::start() {
  UtilityFunctions::print( "GDFujiController::start" );
  active = true;
  acc_time = 0.0f;
  return true;
}

void GDFujiController::toggle() {
  if( active )
    stop();
  else
    start();
}

void GDFujiController::stop() {
  UtilityFunctions::print( "GDFujiController::stop" );
  active = false;
}

void GDFujiController::_process(double delta) {
  if( !active )
    return;

  if( eval_step( &ev ) ) {
    UtilityFunctions::print( "eval_step complete" );
    active = false;
    return;
  }

  acc_time += delta;
  if( acc_time > 1.0f ) {
    UtilityFunctions::print( "GDFujiController::camera_event" );
    acc_time -= 1.0f;
    emit_signal( "camera_event", "Camera says hi" );
  }
}
