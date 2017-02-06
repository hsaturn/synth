# synth
SDL Sound complex generator in command line.

This one file c++ file is able to generate very complicated sound such as a synthetiser.

# examples

440Hz sinus for 1000ms (1s)
> synth 1000 sin 440

Same, but lower
> synth 1000 sin 440:50

220Hz chopped sinus (5x / sec) for 1sec

> synth 1000 am 0 100 sinus 220 square 5

Changing am from square to sinus

> synth 1000 am 0 100 sinus 220 sinus 5

# Mixing two signals, triangle @440Hz 50% volume and sinus 330Hz for 1sec

> synth 1000 tri 440:50 sinus 330


