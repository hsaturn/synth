# synth
SDL Sound complex generator in command line. Like a synthetiser.

Also, a good code sample for SDL AudioDevice real time manipulation.
You, developper may want to use only the end of the synth.cpp file (from void audioCallback(...) to the end).

This one file c++ file is able to generate very complicated sound such as a synthetiser.

# features
Auto mixing all sound generation of this kind :

## Audio features
* sinus
* square
* triangle
* distorsion
* white noise
* reverberation / echo

* frequency modulation (any signal)
* amplitude modulation (any signal)
* attack decay sustain release hold delay enveloppe (adsr)
* custom envelope (from file also)

* left/right cut channel
* mono converter
* sound mixer

* external hooks sound generator (mouse sound demo)

## Misc features

* one command line => very complicated sounds

  see examples for more.

* sound definitions can be stored as file
* enveloppe definitions can be stored in files

* easy to integrate to existing project

Example of a engine noise that accelerates

 > ./synth 30000 reverb 10:50 fm 0 150 am 0 100 triangle 100:50 square 39 adsr 1:0 1000:0 2000:100 5001:400 6000:400 8000:-100 9000:0 loop level 1

 The corresponding description file could be

```
 define engine
 {
	fm
		0 150
		am
			0 100
			triangle 100:50
			square 39
		adsr
			1:0 1000:0 2000:100 5001:400 6000:400 8000:-100 9000:0 loop
			level 1
 }

 reverb 10:50 engine
```

  The base sound is a triangle@100Hz (50% volume so the reverb does not saturate.
  The base sound is chopped @39Hz by the am modulator.

  Then, the result is frequency modulated by the fm modulator.

  The fm modulator takes 4 arguments, in the engine example :
   
* 0 min frequency factor (0 => 0Hz => no sound)
* 140 max frequecy factor (triangle @ 100Hz * 1.5)
* am ... The sound that will be modulated
* adsr any generator, values produced are used to modulate frequency.

  The envelope (adsr) produces values from 0 to 1, and the result is used
  to modify the frequency modulation.

## Hooks (integration in existing software)

One may want to use this engine sound in a game, but how to modulate dynamically the sound from an
existing value (the speed of the engine in the game).

In the example, the fm modulator uses adsr that returns values from 0 to 1 to modulate the frequency.
The idea is then to replace adsr by a callback that will be responsible to return this value.

Easy => replace adsr by a name of your choice, implement the callback and enjoy !

Let say we use the name 'engine_speed' :

```
 define engine
 {
	fm
		0 150
		am
			0 100
			triangle 100:50
			square 39
		engine_speed
 }

 reverb 10:50 engine
```

And here is the C++ code to define engine_speed


```c++
float engine_speed;	// Float value that represents the engine speed
class EngineSpeedHook : public SoundGeneratorVarHook<float>
{
	public:
		EngineSpeedHook(float &v) : SoundGeneratorFloatHook(v, 0.0, 100.0, "engine_speed"){}
};

static EngineSpeedHook instance;	// Needed to register the 'engine_speed' hook so it can be used.
```

One should create more sophisticated hooks. See mouse.cpp for the class that defines mouse_hook.

It 


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
 