extends GDFujiController

signal app_msg( msg : String )

func _ready():
	set_max_time_per_step( 1000 * 15 );

func log( msg : String ):
	app_msg.emit( msg );

