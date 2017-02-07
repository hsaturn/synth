# ADSR Enveloppe generator

Attack / Decay / Sustain / Release

In fact this is more or less an adsr generator.

It can generate normal adsr envelope, shorter ones
such as only ad or ar but also
adds a delay and hold (such as some synthetisers.

There is no limit

syntax:

	adsr ms:vol ms:vol .... last generator

	By default, 0:0 is the first implicit value.

	ms is a time (millisecond)
	vol is the volume reached at that time.

Example

adsr 100:100 200:50 1000:50 1200:0 last

	
