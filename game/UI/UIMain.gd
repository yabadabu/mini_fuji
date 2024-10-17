extends Control

var frame_counter = 0;
var last_timestamp

func _ready():
	$Interval.sampleBegins.connect( onSampleBegins )
	$Interval.sampleEnds.connect( onSampleEnds )
	$Interval.samplesComplete.connect( onSamplesComplete )
	last_timestamp = Time.get_ticks_msec()

func elapsedTime():
	var new_timestamp = Time.get_ticks_msec()
	var delta = new_timestamp - last_timestamp
	last_timestamp = new_timestamp
	return delta * 0.001

func onSampleBegins( idx : int ):
	Fuji.log( "[%1.3f] Sample %d Begins" % [ elapsedTime(), idx ] )

func onSampleEnds( idx : int ):
	Fuji.log( "[%1.3f] Sample %d Ends" % [ elapsedTime(),idx ] )

func onSamplesComplete( ):
	Fuji.log( "[%1.3f] All Samples complete" % [ elapsedTime() ] )

func _on_start_button_pressed():
	#Fuji.toggle()
	last_timestamp = Time.get_ticks_msec()
	$Interval.startIntervals()

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
