extends Control

var frame_counter = 0;

func _on_start_button_pressed():
	Fuji.toggle()

func _process( _delta ):
	frame_counter += 1
	%StatusLabel.text = "frame: %d" % [frame_counter]

func _on_send_pressed():
	var target_addr = %BindEdit.text
	Fuji.send_udp_message( target_addr, 5002, "Godot knocks the door" )

func _on_dump_interfaces_button_pressed():
	var ips : Array = Fuji.get_local_addresses();
	for ip in ips:
		Fuji.log( ip )
