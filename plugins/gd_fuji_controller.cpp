#include "gd_fuji_controller.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// ---------------------------------------------------------------------
void GDFujiController::_bind_methods() {
  ClassDB::bind_method(D_METHOD("start"), &GDFujiController::start);
  ClassDB::bind_method(D_METHOD("stop"), &GDFujiController::stop);
  ClassDB::bind_method(D_METHOD("toggle"), &GDFujiController::toggle);
  ADD_SIGNAL(MethodInfo("camera_event", PropertyInfo(Variant::STRING, "line")));
}

// ---------------
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
  acc_time += delta;
  if( acc_time > 1.0f ) {
    UtilityFunctions::print( "GDFujiController::camera_event" );
    acc_time -= 1.0f;
    emit_signal( "camera_event", "Camera says hi" );
  }
}
