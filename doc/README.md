# ADSR Enveloppe generator

Attack / Decay / Sustain / Release

In fact this is more or less an adsr generator.

It can generate normal adsr envelope, shorter ones
such as only ad or ar but also
adds a delay and hold (such as some synthetisers.

There is no limit

## Syntax:

	adsr ms:vol ms:vol .... type generator

	By default, 0:0 is the first implicit value.

	ms is a time (millisecond)
	vol is the volume reached at that time.
	type is either once or loop

Example

adsr 100:100 200:50 1000:50 1200:0 last

# AVCRegulator

Automatic volume regulator

When saturation occurs, this modules tries to adjust the volume in order to reduce it.

## Syntax
	avc speed generator

	speed : float [0..1], speed at which volume is changed.
 
Note : yet, speed is not time calibrated. This may change in future.

## Example

avc 1 sinus 220:200


	
