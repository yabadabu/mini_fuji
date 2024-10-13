extends Control

@onready var fuji : GDFujiController = $GDFujiController
var frame_counter = 0;

func _ready():
	fuji.camera_event.connect( addLog )
	fuji.camera_log.connect( addLog )
	fuji.set_max_time_per_step( 1000 * 3 );

func addLog( new_text : String ):
	var line : Label = Label.new()
	line.text = new_text
	%LogLines.add_child( line )

func _on_clear_button_pressed():
	while %LogLines.get_child_count():
		%LogLines.remove_child( %LogLines.get_child( %LogLines.get_child_count() - 1))

func _on_start_button_pressed():
	fuji.toggle()
	addLog( "New line added %d" % [%LogLines.get_child_count()])

func _process( _delta ):
	frame_counter += 1
	%StatusLabel.text = "frame: %d" % [frame_counter]

func _on_send_pressed():
	var target_addr = %BindEdit.text
	addLog( "GD.addr %s" % [target_addr] )
	var rc = fuji.send_udp_message( target_addr, 5002, "Godot knocks the door" )
	addLog( "Sending udp msg to %s => %d " % [target_addr, rc] )

func _on_dump_interfaces_pressed():
	var ips : Array = fuji.get_local_addresses();
	for ip in ips:
		addLog( ip )
