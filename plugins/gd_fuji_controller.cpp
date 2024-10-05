#include "gd_fuji_controller.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
  #include "actions_list.h"
}

using namespace godot;

// ---------------------------------------------------------------------
void GDFujiController::_bind_methods() {
  ClassDB::bind_method(D_METHOD("start"), &GDFujiController::start);
  ClassDB::bind_method(D_METHOD("stop"), &GDFujiController::stop);
  ClassDB::bind_method(D_METHOD("toggle"), &GDFujiController::toggle);
  ADD_SIGNAL(MethodInfo("camera_event", PropertyInfo(Variant::STRING, "line")));
}

void on_event(void* context, const char* str ) {
  if( !context )
    return;
  GDFujiController* c = (GDFujiController*) context;
  c->onEvent( str );
}

// ---------------
GDFujiController::GDFujiController() {
  is_valid = conn_create( &conn );
  conn.on_event = (callback_event_t) { .context = this, .callback = &on_event };
  eval_create( &ev, &conn, actions_take );
}

GDFujiController::~GDFujiController() {
  if( is_valid )
    conn_destroy( &conn );
}

void GDFujiController::onEvent( const char* msg ) {
  String str( msg );
  UtilityFunctions::print( msg );
  emit_signal( "camera_event", str );
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
