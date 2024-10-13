extends VBoxContainer

func _ready():
	Fuji.camera_event.connect( addLog )
	Fuji.camera_log.connect( addLog )
	Fuji.app_msg.connect( addLog )
	_on_clear_button_pressed()

func addLog( new_text : String ):
	var line : Label = Label.new()
	line.text = new_text
	%LogLines.add_child( line )

func _on_clear_button_pressed():
	while %LogLines.get_child_count():
		%LogLines.remove_child( %LogLines.get_child( %LogLines.get_child_count() - 1))
