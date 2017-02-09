#include <libsynth.hpp>


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

	while(input.good())
	{
		SoundGenerator* g = SoundGenerator::factory(input);
		SoundGenerator::play(g);
	}

	if (SoundGenerator::count()==0)
	{
		cerr << "Parsing ok, but not generator list is empty" << endl;
		exit(1);
	}

	SDL_Delay(duration); // Play for ms

	return 0;
}
