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
  ClassDB::bind_method(D_METHOD("send_udp_message"), &GDFujiController::send_udp_message);
  ClassDB::bind_method(D_METHOD("get_local_addresses"), &GDFujiController::get_local_addresses);

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
  if( nchars > 0 && msg[ nchars - 1 ] == '\n')
    ((char*) msg)[ nchars - 1 ] = 0x00;
  UtilityFunctions::print( msg );
}

// ---------------
GDFujiController::GDFujiController() {
  setDbgCallback( this, DbgTrace, &on_dbg_msg );
  is_valid = conn_create( &conn );
  
  conn.on_event.context = this;
  conn.on_event.callback = &on_event;
  conn.on_progress.context = this;
  conn.on_progress.callback = &on_download_progress;
  conn.on_progress.enabled = false;
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
  }

}

int GDFujiController::send_udp_message( const String& broadcast, int port, const String& msg) {
  CharString msg_utf8 = msg.utf8();
  CharString broadcast_utf8 = broadcast.utf8();
  const char* c_msg = msg_utf8.get_data();
  const char* c_broadcast = broadcast_utf8.get_data();
  channel_t ch;
  if( !ch_create( &ch, "udp:0.0.0.0", port ) )
    return -100;
  int rc = ch_broadcast( &ch, c_msg, strlen( c_msg ), c_broadcast );
  ch_close( &ch );
  return rc;
}

Array GDFujiController::get_local_addresses() {
  Array arr;
  network_interface_t net_ifs[16];
  int n = ch_get_local_network_interfaces( net_ifs, 16 );
  for( int i=0; i<n; ++i ) {
    String ip( net_ifs[i].ip );
    arr.append( ip );
  }
  return arr;
}
