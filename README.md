# synth
SDL Sound complex generator in command line. Like a synthetiser.

Also, a good code sample for SDL AudioDevice real time manipulation.
You, developper may want to use only the end of the synth.cpp file (from void audioCallback(...) to the end).

This one file c++ file is able to generate very complicated sound such as a synthetiser.

# features
Auto mixing all sound generation of this kind :

sinus
square
triangle
distorsion
frequency modulation (any signal)
amplitude modulation (any signal)

# examples

440Hz sinus for 1000ms (1s)
> synth 1000 sin 440

Same, but lower
> synth 1000 sin 440:50

220Hz chopped sinus (5x / sec) for 1sec

> synth 1000 am 0 100 sinus 220 square 5

Changing am from square to sinus

> synth 1000 am 0 100 sinus 220 sinus 5

Mixing two signals, triangle @440Hz 50% volume and sinus 330Hz for 1sec

> synth 1000 tri 440:50 sinus 330

... Hum how to define that ???

 > synth 1000000 am 0 100 fm 80 120 sq 440:25 tri 1 square 5 am 0 100 fm 80 120 sq 330:25 tri 1 square 6 fm 80 120 sinus 1200:30 sinus 3
 
 How to group together generators (to be used by a modulator for example)
 
 > synth { sinus 220 sinus 330 }
 > synth fm 80 120 { sinus 1500 sinus 1300 } sinus 5
 

