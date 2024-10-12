#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/binder_common.hpp>

extern "C" {
  #include "connection.h"
  #include "actions.h"
}

namespace godot {

// -------------------------------------------------
class GDFujiController : public Node {
  GDCLASS(GDFujiController, Node)

protected:
  static void _bind_methods();

  bool  is_valid = false;
  bool  active = false;
  float acc_time = 0.0f;
  
  conn_t       conn;
  evaluation_t ev;

public:
  GDFujiController();
  ~GDFujiController();

  bool start();
  void stop();
  void toggle();
  void _process(double delta) override;
  void set_max_time_per_step( int new_max_time_per_step_usecs );
  int send_udp_message( const String& address, int port, const String& msg);
};

}
