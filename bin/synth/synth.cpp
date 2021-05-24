#include <libsynth.hpp>
#include "unistd.h"

void help()
{
	cout << "Syntax : " << endl;
	cout << "  synth [duration] [sample_freq] generator_1 [generator_2 [...]]" << endl;
	cout << endl;
	cout << "  duration     : sound duration (ms)" << endl;
	cout << endl;
	cout << "Available sound generators : " << endl;
	cout << "  file.synth : read synth file" << endl;

	SoundGenerator::help();

	exit(1);
}

int main(int argc, const char* argv[])
{
	long duration;
	int i(1);
	stringstream input;

	if (argc<2)
		help();

	duration = atol(argv[i]);
	if (duration == 0)
		duration=10000;
	else
		i++;

	for(; i<argc; i++)
	{
		string arg(argv[i]);
		if (arg=="help" || arg=="-h")
			help();
		else
			input << arg << ' ';
	}

    SoundGenerator::setVolume(0);   // Avoid sound clicks at start
	SoundGenerator::fade_in(10);

	bool needed = true;
	while(input.good())
	{
		SoundGenerator* g = SoundGenerator::factory(input, needed);
		SoundGenerator::play(g);
		needed = false;
	}

	if (SoundGenerator::count()==0)
	{
		cerr << "Parsing ok, but generator list is empty" << endl;
		exit(1);
	}
	//else
	//	cout << "Playing, sounds count = " << SoundGenerator::count() << endl;

	const int fade_time=50;

    if (duration > fade_time)
    {
	    SDL_Delay(duration-fade_time); // Play for ms
	    cout << "Fading out" << endl;
	    SoundGenerator::fade_out(fade_time);
	    SDL_Delay(fade_time); // Play for 100 ms (while fade out)
    }
    else
    {
	    cout << "Fading out direct" << endl;
	    SoundGenerator::fade_out(fade_time);
	    SDL_Delay(fade_time); // Play for ms (while fading out)
    }
	    SDL_Delay(1000); // Wait till the end of buffer is played (avoid clicks) TODO this is buffer size dependant

	return 0;
}
