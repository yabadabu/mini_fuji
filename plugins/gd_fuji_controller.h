#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/binder_common.hpp>

namespace godot {

// -------------------------------------------------
class GDFujiController : public Node {
  GDCLASS(GDFujiController, Node)

protected:
  static void _bind_methods();

  bool active = false;
  float acc_time = 0.0f;

public:
  bool start();
  void stop();
  void toggle();
  void _process(double delta) override;

};

}
