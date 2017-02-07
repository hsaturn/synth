all: synth mousynth

synth: synth.cpp
	g++ -Wall -O3 -std=c++11 synth.cpp -o synth -lSDL2 -lSDL_mixer

mousynth: mouse.cpp synth.cpp
	g++ -Wall -O3 -std=c++11 mouse.cpp -o mousynth -lSDL2 -lSDL_mixer

test: synth
	./tests/tests.sh
	
clean:
	rm -f synth
	rm -f mousynth

