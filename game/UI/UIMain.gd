extends Control

@onready var fuji := $GDFujiController

func _ready():
	fuji.camera_event.connect( addLog )

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
