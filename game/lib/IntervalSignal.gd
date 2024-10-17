extends Timer
class_name IntervalSignal

var num_samples := 3
var initial_delay := 2.0
var sample_duration := 3.0
var time_between_samples := 1.0

var curr_sample := 0
var waiting_sample := false

signal sampleBegins( sample_idx : int )
signal sampleEnds( sample_idx : int )
signal samplesComplete

func _ready():
	timeout.connect( nextTimer )

func startIntervals( ):
	curr_sample = -1
	waiting_sample = true
	nextTimer()
	
func nextTimer():

	var next_delay := 0.0
	
	if waiting_sample:
		if curr_sample >= 0:
			sampleEnds.emit( curr_sample )
			next_delay = time_between_samples
		else:
			next_delay = initial_delay
	
		curr_sample = curr_sample + 1
		waiting_sample = false
		
		if curr_sample == num_samples:
			samplesComplete.emit()
			stop()
			return
	
	else:
		sampleBegins.emit( curr_sample )
		next_delay = sample_duration
		waiting_sample = true
	
	start( next_delay )
	
func getTimeForNextSample():
	if waiting_sample:
		if curr_sample == 0:
			return initial_delay
		return time_between_samples
	return sample_duration


