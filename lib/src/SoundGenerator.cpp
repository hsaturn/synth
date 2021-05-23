#include "libsynth.hpp"
#include <algorithm>

using namespace std;

extern uint32_t samples_per_seconds;


// Frequencies list
static map<string, sgfloat > sf;

SDL_AudioSpec SoundGenerator::have;

SoundGenerator* SoundGenerator::factory(string s)
{
	stringstream stream;
	stream << s;
	return factory(stream);
}

SoundGenerator* SoundGenerator::factory(istream& in, bool needed)
{
	SoundGenerator* gen = 0;
	last_type = "";
	string type;
	while (in.good())
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
	else if (type == "-v")
	{
		verbose++;
		return factory(in, needed);
	}
	else if (type == "-b")
	{
		uint32_t buffer_size;
		in >> buffer_size;
		if (init_done)
			cerr << "Unable to change buffer length once sound is played. :-(" << endl;
		else
			wanted_buffer_size = buffer_size;
		cout << "WB" << wanted_buffer_size << endl;

		return factory(in, needed);
	}
	else if (type == "-s")
	{
		uint32_t spf;
		in >> spf;
		if (init_done)
			cerr << "Unable to change samples per second once sound engine has started. :-(" << endl;
		else
			samples_per_seconds = spf;

		return factory(in, needed);
	}
	else if (type == "define")
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
			} while (brackets && in.good());
			defines[name] = define.str();
			gen = factory(in, needed);
		}
		else
		{
			cerr << "libsynth, ERROR missing name" << endl;
			exit(1);
		}
	}
	else if (type.length())
	{
		last_type = type;
		if (generators.find(type) != generators.end())
		{
			gen = factory(type, in);
			if (gen == 0)
			{
				cerr << "libsynth, ERROR Unable to build " << last_type << endl;
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
				gen = factory(def, false);
				if (gen == 0)
				{
					cerr << "libsynth, ERROR Unable to build " << last_type << ", please fix the corresponding define." << endl;
					exit(1);
				}
			}
		}
	}
	if (needed)
	{
		if (gen == 0)
			missingGeneratorExit("");
		else if (!gen->isValid())
		{
			cerr << "libsynth, ERROR Deleting invalid generator (" << gen->name << ')' << endl;
			delete gen;
			gen = 0;
		}
	}
	return gen;
}

SoundGenerator* SoundGenerator::factory(const std::string type, istream& in)
{
	if (type=="")
	{
		return factory(in);
	}
	auto it = generators.find(type);
	if (it != generators.end())
	{
		SoundGenerator* generator=it->second->build(in);
		if (generator) generator->name = type;
		return generator;
	}
	else
		return nullptr;
}

SoundGenerator::SoundGenerator(string name)
{
	while (name.length())
	{
		if (name.find(' ') == string::npos)
		{
			generators[name] = this;
			name = "";
		}
		else
		{
			string n = name.substr(0, name.find(' '));
			if (n.length())
				generators[n] = this;
			name.erase(0, name.find(' ') + 1);
		}
	}
}

void SoundGenerator::audioCallback(void *unused, Uint8 *byteStream, int byteStreamLength)
{
	mtx.lock();
	uint32_t ech = byteStreamLength / sizeof (int16_t);
	int16_t* stream =  reinterpret_cast<int16_t*> ( byteStream );
	uint32_t i;

	for (i = 0; i < ech; i += 2)
	{
		if (list_generator_size)
		{
			sgfloat  left = 0;
			sgfloat  right = 0;

			for (auto generator : list_generator)
				generator->next(left, right);

			if (left > 1.0)
			{
				left = 1;
				saturate = true;
			}
			else if (left<-1.0)
			{
				left = -1;
				saturate = true;
			}
			if (right > 1.0)
			{
				right = 1;
				saturate = true;
			}
			else if (right<-1.0)
			{
				right = -1;
				saturate = true;
			}
			stream[i] = 32767 * left / list_generator_size;
			stream[i + 1] = 32767 * right / list_generator_size;
		}
		else
		{
			stream[i] = 0;
			stream[i + 1] = 0;
		}
	}
	mtx.unlock();
}

bool SoundGenerator::init()
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
	SDL_AudioSpec want;

	SDL_memset(&want, 0, sizeof (want)); /* or SDL_zero(want) */
	want.freq = samples_per_seconds;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = wanted_buffer_size;
	want.callback = audioCallback;

	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (dev == 0)
	{
		SDL_Log("Failed to open audio: %s", SDL_GetError());
	}
	else
	{
		if (have.format != want.format)
		{ /* we let this one thing change. */
			SDL_Log("We didn't get Float32 audio format.");
		}
		SDL_PauseAudioDevice(dev, 0); /* start audio playing. */
	}
	buf_size = have.samples;
	samples_per_seconds = have.freq;
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

bool SoundGenerator::readFrequencyVolume(istream& in)
{
	bool bRet;

	if (sf.size() == 0)
	{
		ifstream notes("frequencies.def");
		while (notes.good())
		{
			string row;
			getline(notes, row);
			stringstream note;
			note << row;

			sgfloat  freq;
			note >> freq;
			if (freq)
			{
				string name;
				while (note.good())
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

	if (s.length() == 0)
		return false;

	if (s.find(':') != string::npos)
	{
		note = s.substr(0, s.find(':'));
		s.erase(0, s.find(':') + 1);
		bRet = setValue("v", s);
	}
	else
	{
		note = s;
		bRet = setValue("v", 100);
	}

	bRet &= setValue("f", note);

	return bRet;
}

string SoundGenerator::getTypes()
{
	string s;
	for (auto it : generators)
		s += it.first + ' ';
	return s;
}

sgfloat  SoundGenerator::rand()
{
	return (sgfloat ) ::rand() / ((sgfloat ) RAND_MAX / 2) - 1.0;
}

SoundGenerator::HelpEntry* SoundGenerator::addHelpOption(HelpEntry* entry) const
{
	entry->addOption(new HelpOption("freq:level", "Frequency and level (%)", HelpOption::FREQ_VOL));
	return entry;
}

void SoundGenerator::help()	// @FIXME memory leak if called many times
{
	Help help;
	help.add(new HelpEntry("-b", "Change sound buffer length, default: " + to_string(wanted_buffer_size)));
	help.add(new HelpEntry("-s", "Number of samples per seconds, default: " + to_string(samples_per_seconds)));

	map<const SoundGenerator*, bool>	done;
	for (auto generator : generators)
	{
		if (done.find(generator.second) == done.end())
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
		cerr << "libsynth, ERROR Unknown generator type (" << last_type << ")" << endl;
	else
		cerr << "libsynth, ERROR Missing generator" << endl;
	if (msg.length())
		cerr << msg;
	exit(1);
}

void SoundGenerator::help(Help& help) const
{
	cout << "freq[:volume] (@FIXME not a right place)" << endl;	// FIXME

}

bool SoundGenerator::has(SoundGenerator* generator, bool lock)
{
	bool bRet;
	if (lock) mtx.lock();
	auto it = find(list_generator.begin(), list_generator.end(), generator);
	if (it == list_generator.end())
		bRet = false;
	else
		bRet = true;
	if (lock) mtx.unlock();
	return bRet;
}

void SoundGenerator::play(SoundGenerator* generator)
{
	init();
	if (generator == 0)
		return;
	if (generator->isValid())
	{
		mtx.lock();
		if (has(generator, false) == false)
			list_generator.push_back(generator);
		list_generator_size = list_generator.size();
		mtx.unlock();
	}
	else
		cerr << "libsynth ERROR: skipping invalid generator play." << endl;
}

bool SoundGenerator::stop(SoundGenerator* generator)
{

	cerr << "libsynth, WARNING: STOP NOT IMPLEMENTED" << endl;
	remove(generator);
	return false;
}

bool SoundGenerator::remove(SoundGenerator* generator)
{
	bool bRet = false;
	mtx.lock();
	if (has(generator, false))
	{
		bRet = true;
		list_generator.remove(generator);
		list_generator_size = list_generator.size();
	}
	else
		cerr << "libsynth, WARNING : Unable to remove sound generator " << generator << ", size=" << list_generator_size << endl;
	mtx.unlock();
	return bRet;
}

ostream& operator << (ostream& out, const SoundGenerator::Help &help)
{
	string::size_type cmd = 0;

	for (auto entry : help.entries)
	{
		/* if (cmd < entry->getFullCmd().length())
			cmd = entry->getFullCmd().length();
		 */
	}
	cmd++;
	for (auto entry : help.entries)
	{
		out << SoundGenerator::Help::padString(entry->getFullCmd(), cmd) << " : ";
		out << entry->getDesc() << endl;

		string::size_type l = 0;
		for (auto option : entry->getOptions())
		{
			string opt = option->str();
			if (opt.length() > l)
				l = opt.length();
		}

		// Display options (vertical alignment))
		for (auto option : entry->getOptions())
		{
			string opt = option->str();
			out << "     " << SoundGenerator::Help::padString("", cmd) << "   ";
			out << SoundGenerator::Help::padString(opt, l) << " : " << option->getDesc() << endl;
		}

		if (entry->getExample().length())
		{
			out << SoundGenerator::Help::padString("", cmd) << "   ";
			out << SoundGenerator::Help::padString("Example", l + 5) << " : " << entry->getExample() << endl;
		}

		out << endl;
	}

	return out;
}

string SoundGenerator::Help::padString(string s, string::size_type length)
{
	while (s.length() < length)
		s += ' ';
	return s;
}

string SoundGenerator::HelpEntry::concatOptions() const
{
	string concat = "";
	for (auto option : options)
	{
		if (concat.length())
			concat += ' ';
		concat += option->str();
	}
	return concat;
}

string SoundGenerator::HelpEntry::getFullCmd() const
{
	string options(concatOptions());

	string fullCmd(cmd + ' ');
	string::size_type p = fullCmd.find('$');
	if (p != string::npos)
		fullCmd = fullCmd.substr(0, p) + options + fullCmd.substr(p + 1);
	else
		fullCmd = fullCmd + options;
	return fullCmd;
}

string SoundGenerator::HelpOption::str() const
{
	string str;
	if (isOptional())
		str = '[' + getName();
	else
		str = getName();

	if (isRepeatable())
		str += " [" + getName() + "_2 ...]";
	if (isOptional())
		str += ']';

	return str;
}

bool SoundGenerator::setValue(string name, sgfloat  value)
{
	stringstream in;
	in << value;
	return setValue(name, in);
}

bool SoundGenerator::setValue(string name, string value)
{
	stringstream in;
	in << value;
	return setValue(name, in);
}

bool SoundGenerator::setValue(string name, istream &in)
{
	bool bRet = false;
	stringstream in2;
	if (in2.good())
	{
		if (name == "v")
		{
			in >> volume;
			in2 << volume;

			volume /= 100.0;

			bRet = true;
		}
		else if (name == "f")
		{
			string note;
			in >> note;

			if (sf.find(note) != sf.end())
				note = std::to_string(sf[note]);

			freq = atof(note.c_str());

			in2 << note;
			bRet = true;
		}
	}
	if (bRet)
		_setValue(name, in2);
	else
		_setValue(name, in);

	return bRet;
}

bool SoundGenerator::_setValue(string name, istream& value)
{
	cerr << "libsynth WARNING: _setValue(" << name << ", istream&) not handled." << endl;
	return false;
}

bool SoundGenerator::eatWord(istream& in, string expected)
{
	if (!in.good())
		return false;

	stringstream::pos_type last = in.tellg();
	string word;
	in >> word;

	if (word == expected)
		return true;

	in.clear();
	in.seekg(last);
	return false;
}

sgfloat  SoundGenerator::readFloat(istream& in, sgfloat  min, sgfloat  max, string varname)
{
	char c;
	if (min > max)
	{
		cerr << "DEV ERROR: min > max " << min << "/" << max << " for " << varname << endl;
		return max;
	}
	if (in.good())
	{
		c = trim(in);
		if (c != '.' && (c < '0' || c > '9'))
		{
			cerr << "ERROR: Expecting float for '" << varname << "' value.";
			return max;
		}
		sgfloat  f;
		in >> f;
		if (f > max || f < min) cerr << "WARNING: " << varname << '=' << f << " out of range [" << min << "-" << max << "] !" << endl;
		if (f < min) f = min;
		if (f > max) f = max;

		if (verbose)
			cout << varname << '=' << f << endl;
		return f;
	}
	else
		cerr << "WARNING: Missing " << varname << " parameter [" << min << '-' << max << "]" << endl;

	return max;

}

sgfloat SoundGenerator::readFrequency(istream& in, string name)
{
	sgfloat f;
	in >> f;
	if (f < 0 || f > 200000.0)
		cerr << "ERROR: Invalid " << name << " frequency : " << f << endl;
	if (verbose)
		cout << name << '=' << f << endl;
	return f;
}

char SoundGenerator::trim(istream& in)
{
	if (!in.good())
		return 0;
	char c = in.peek();
	while ((c == ' ' || c == 10 || c == 13 || c == '\t') && in.good())
	{
		in.ignore();
		c = in.peek();
	}
	return c;
}
