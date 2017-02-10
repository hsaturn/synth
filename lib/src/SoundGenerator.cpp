#include "libsynth.hpp"

extern uint32_t ech;

SoundGenerator* SoundGenerator::factory(istream& in, bool needed)
{
	init();
	SoundGenerator* gen=0;
	last_type="";
	string type;
	while(in.good())
	{
		in >> type;
		if (type.length() && type[0] != '#')
			break;
		else
		{
			string s;
			getline(in, s);
		}
	}
	
	if (type.find(".synth") != string::npos)	// assume a file
		{
		ifstream file(type);
		if (file.good())
			gen = factory(file, needed);
	}
	else if (type == "print")
	{
		string line;
		getline(in, line);
		if (echo)
			cout << line << endl;
		gen = factory(in, needed);
	}
	else if (type == "-q")
	{
		echo = false;
	}
	else if (type == "-b")
	{
		unsigned long buf_length;
		in >> buf_length;
		cerr << "Unable to modify buf_length now :-(" << endl;
		return factory(in, needed);
	}
	else if (type=="define")
	{
		string name;
		in >> name;
		if (name.length())
		{
			uint16_t brackets = 0;
			stringstream define;
			do
			{
				string item;
				in >> item;
				if (item == "{")
					brackets++;
				else if (item == "}")
					brackets--;
				else if (brackets == 0)
				{
					string end_of_line;
					getline(in, end_of_line);
					define << end_of_line << endl;
					break;
				}
				define << item << ' ';
				cout << "(" << item << ") ";
			} while (brackets && in.good());
			defines[name] = define.str();
			
			gen = factory(in, needed);
		}
		else
		{
			cerr << "missing name" << endl;
			exit(1);
		}
	}
	else if (type.length())
	{
		last_type = type;
		auto it = generators.find(last_type);
		if (it != generators.end())
		{
			gen = it->second->build(in);
			if (gen == 0)
			{
				cerr << "Unable to build " << last_type << endl;
				exit(1);
			}
		}
		else
		{
			auto it = defines.find(last_type);
			if (it != defines.end())
			{
				stringstream def;
				def << defines[last_type];
				gen = factory(def,false);
				if (gen == 0)
				{
					cerr << "Unable to build " << last_type << ", pleasee fix the corresponding define." << endl;
					exit(1);
				}
			}
		}
		if (gen == 0 && needed)
			cerr << "Unknown generator : " << type << endl;
	}
	if (gen == 0)
	{
		if (needed)
			missingGeneratorExit("");
	}
	return gen;
}

SoundGenerator::SoundGenerator(string name)
{
	while(name.length())
	{
		if (name.find(' ')==string::npos)
		{
			generators[name] = this;
			name="";
		}
		else
		{
			string n = name.substr(0, name.find(' '));
			if (n.length())
				generators[n] = this;
			name.erase(0, name.find(' ')+1);
		}
	}
}

void SoundGenerator::audioCallback(void *unused, Uint8 *byteStream, int byteStreamLength)
{
	mtx.lock();
	if (list_generator_size)
	{
		uint32_t ech = byteStreamLength / sizeof(int16_t);
		int16_t* stream =  reinterpret_cast<int16_t*>( byteStream );
		uint32_t i;

		for (i = 0; i < ech; i+=2)
		{
			float left = 0;
			float right = 0;
			for(auto generator: list_generator)
				generator->next(left, right);

			if (left>1.0)
			{
				left=1;
				saturate = true;
			}
			else if (left<-1.0)
			{
				left=-1;
				saturate = true;
			}
			if (right>1.0)
			{
				right=1;
				saturate = true;
			}
			else if (right<-1.0)
			{
				right=-1;
				saturate = true;
			}
			stream[i] = 32767 * left / list_generator_size;
			stream[i+1] = 32767 * right / list_generator_size;
		}
	}
	mtx.unlock();
}

bool SoundGenerator::init(uint16_t wanted_buffer_size)
{
	if (init_done)
		return true;
	buf_size = wanted_buffer_size;
	if (SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		SDL_Log("Error while initializing sdl audio subsystem.");
		return false;
	}
	dev = 0;
	std::atexit(SoundGenerator::quit);
	SDL_AudioSpec want, have;

	SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
	want.freq = ech;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = 1024;
	want.callback = audioCallback;

	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (dev == 0) {
		SDL_Log("Failed to open audio: %s", SDL_GetError());
	} else {
		if (have.format != want.format) { /* we let this one thing change. */
			SDL_Log("We didn't get Float32 audio format.");
		}
		SDL_PauseAudioDevice(dev, 0); /* start audio playing. */
	}
	buf_size = have.samples;
	init_done = true;
	return true;
}

void SoundGenerator::quit()
{
	mtx.lock();
	// FIXME unallocate list_generator ???
	// but what if this is not us that have allocated them ?
	list_generator.clear();
	SDL_QuitSubSystem(SDL_INIT_AUDIO | SDL_INIT_TIMER);
	mtx.unlock();
}

void SoundGenerator::close()
{
	SDL_CloseAudioDevice(dev);
	init_done = false;
}

SoundGenerator::SoundGenerator(istream& in)
{
	static map<string, float> sf;
	
	if (sf.size()==0)
	{
		ifstream notes("frequencies.def");
		while(notes.good())
		{
			string row;
			getline(notes, row);
			stringstream note;
			note << row;
			
			float freq;
			note >> freq;
			if (freq)
			{
				string name;
				while(note.good())
				{
					note >> name;
					sf[name] = freq;
				}
			}
		}
	}
	string s;
	string note;
	in >> s;
	
	if (s.find(':')!=string::npos)
	{
		note = s.substr(0,s.find(':'));
		s.erase(0, s.find(':')+1);
		volume = atof(s.c_str())/100;
	}
	else
	{
		note = s;
		volume = 1;
	}

	if (sf.find(note) != sf.end())
		freq = sf[note];
	else
		freq = atof(note.c_str());
	
	if (freq<=0 || freq>30000)
	{
		cerr << "Invalid frequency " << note << endl;
		exit(1);
	}

	if (volume<-2 || volume>2)
	{
		cerr << "Invalid volume " << volume*100 << endl;
		exit(1);
	}
}

string SoundGenerator::getTypes() {
	string s;
	for (auto it : generators)
		s += it.first + ' ';
	return s;
}

float SoundGenerator::rand()
{
	return (float)::rand() / ((float)RAND_MAX/2) -1.0;
}

SoundGenerator::HelpEntry* SoundGenerator::addHelpOption(HelpEntry* entry) const
{
	entry->addOption(new HelpOption("freq:level", "Frequency and level (%)"));
	return entry;
}

void SoundGenerator::help()
{
	Help help;
	map<const SoundGenerator*, bool>	done;
	for(auto generator : generators)
	{
		if (done.find(generator.second)==done.end())
		{
			done[generator.second] = true;
			generator.second->help(help);
		}
	}
	cout << help << endl;
}

void SoundGenerator::missingGeneratorExit(string msg)
{
	if (last_type.length())
		cerr << "Unknown generator type (" << last_type << ")" << endl;
	else
		cerr << "Missing generator" << endl;
	if (msg.length())
		cerr << msg;
	exit(1);
}

void SoundGenerator::help(Help& help) const
{
	cout << "freq[:volume] (not in help system)" << endl;;
}

void SoundGenerator::play(SoundGenerator* generator)
{
	if (generator == 0)
		return;
	mtx.lock();
	list_generator.push_front(generator);
	list_generator_size = list_generator.size();
	mtx.unlock();
}

bool SoundGenerator::stop(SoundGenerator* generator)
{
	cerr << "STOP NOT IMPLEMENTED" << endl;
	return false;
}

bool SoundGenerator::remove(SoundGenerator* generator)
{
	bool bRet = false;
	mtx.lock();
	for(auto it: list_generator)
	{
		if (it == generator)
		{
			bRet = true;
			list_generator.remove(it);
			break;
		}
	}
	mtx.unlock();
	return bRet;
}

ostream& operator << (ostream& out, const SoundGenerator::Help &help)
{
	string::size_type cmd = 0;
	
	for(auto entry: help.entries)
	{
		string options(entry->concatOptions());
		if (entry->getCmd().length() + options.length() > cmd)
			cmd = entry->getCmd().length() + options.length();
	}
	cmd++;
	for(auto entry: help.entries)
	{
		out << SoundGenerator::Help::padString(entry->getCmd() + ' ' + entry->concatOptions(), cmd) << " : ";
		out << entry->getDesc() << endl;
		
		string::size_type l=0;
		for(auto option : entry->getOptions())
		{
			string opt = option->getName();
			if (option->isOptional())
				opt = '['+opt+']';
			if (opt.length()>l)
				l = opt.length();
		}
		for(auto option : entry->getOptions())
		{
			string opt = option->getName();
			if (option->isOptional())
				opt = '['+opt+']';
			out << "     " << SoundGenerator::Help::padString("", cmd) << "   ";
			out << SoundGenerator::Help::padString(opt, l) << " : " << option->getDesc() << endl;
		}
		
		if (entry->getExample().length())
		{
			out << SoundGenerator::Help::padString("", cmd) << "   ";
			out << SoundGenerator::Help::padString("Example", l) << " : " << entry->getExample() << endl;
		}
		
		out << endl;
	}

	return out;
}

string SoundGenerator::Help::padString(string s, string::size_type length)
{
	while(s.length() < length)
		s += ' ';
	return s;
}

string SoundGenerator::HelpEntry::concatOptions() const
{
	string concat="";
	for(auto option : options)
	{
		if (concat.length())
			concat +=' ';
		if (option->isOptional())
			concat += '['+option->getName()+']';
		else
			concat += option->getName();
	}
	return concat;
}

