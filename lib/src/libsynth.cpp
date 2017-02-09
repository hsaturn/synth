/* 
 * File:   libsynth.hpp
 * Author: hsaturn
 *
 * Created on 7 février 2017, 23:47
 */

#ifndef LIBSYNTH_HPP
#define LIBSYNTH_HPP

#include "libsynth.hpp"
#include <cstdlib>

map<string, const SoundGenerator*> SoundGenerator::generators;
string					SoundGenerator::last_type;
map<string, string>		SoundGenerator::defines;
bool					SoundGenerator::echo=true;
bool					SoundGenerator::init_done = false;
SDL_AudioDeviceID		SoundGenerator::dev;
list<SoundGenerator*>	SoundGenerator::list_generator;
uint16_t				SoundGenerator::list_generator_size;	// avoid mx use
mutex					SoundGenerator::mtx;
uint16_t				SoundGenerator::buf_size;
bool					SoundGenerator::saturate = false;

// Auto register for the factory
static SquareGenerator gen_sq;
static TriangleGenerator gen_tr;
static SinusGenerator gen_si;
static AmGenerator gen_am;
static DistortionGenerator gen_dist;
static FmGenerator gen_fm;
static MixerGenerator gen_mix;
static WhiteNoiseGenerator gen_wn;
static EnvelopeSound gen_env;
static LeftSound gen_left;
static RightSound gen_right;
static AdsrGenerator gen_adrs;
static ReverbGenerator gen_reverb;
static LevelSound gen_level;
static MonoGenerator gen_mono;
static AvcRegulator gen_avc;

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
	if (list_generator_size == 0)
		return;
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
	string s;
	in >> s;
	freq = atof(s.c_str());
	if (s.find(':')!=string::npos)
	{
		s.erase(0, s.find(':')+1);
		volume = atof(s.c_str())/100;
	}
	else
		volume = 1;

	if (freq<=0 || freq>30000)
	{
		cerr << "Invalid frequency " << freq << endl;
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
	cout << "freq[:volume] (not in help system)";
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
			out << SoundGenerator::Help::padString("", cmd) << "   ";
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


TriangleGenerator::TriangleGenerator(istream& in)
:
SoundGenerator(in) {
	a = 0;
	da = 4 / ((float) ech / freq);
	sign = 1;
}

void TriangleGenerator::next(float& left, float& right, float speed)
{
	a += da * speed * (float)sign;
	float dv=a;
	if (dv>1)

	{
		sign = -sign;
		dv=1;
	}
	else if (dv<-1)
	{
		dv=-1;
		sign = -sign;
	}
	left += (float)dv * volume;
	right += (float)dv * volume;
}

void TriangleGenerator::help(Help& help) const
{
	help.add(SoundGenerator::addHelpOption(new HelpEntry("triangle","triangle noise")));
}


SquareGenerator::SquareGenerator(istream& in)
: SoundGenerator(in) {
	a = 0;
	da = 1.0f / (float) ech;
	val = 1;
	invert = (float) (ech >> 1) / freq;
}

void SquareGenerator::next(float& left, float& right, float speed)
{
	a += speed;
	left += (float)val * volume;
	right += (float)val * volume;
	if (a > invert)
	{
		a -= invert;
		val = -val;
	}
}

void SquareGenerator::help(Help& help) const
{
	help.add(SoundGenerator::addHelpOption(new HelpEntry("square","square sound")));
}

SinusGenerator::SinusGenerator(istream& in)
: SoundGenerator(in) {
	a = 0;
	da = (2 * M_PI * freq) / (float) ech;
}

void SinusGenerator::next(float& left, float& right, float speed)
{
	static int count=0;
	count++;
	a += da * speed;
	float s = sin(a);
	left += volume * s;
	right += volume * s;
	if (a > 2*M_PI)
	{
		count = 0;
		a -= 2*M_PI;
	}
}

void SinusGenerator::help(Help& help) const
{
	help.add(addHelpOption(new HelpEntry("sinus","sinus wave")));
}

DistortionGenerator::DistortionGenerator(istream& in) {
	in >> level;
	if (in.good()) generator = factory(in, true);

	level = level / 100 + 1.0f;
}

void DistortionGenerator::next(float& left, float& right, float speed)
{
	float l,r;
	generator->next(l,r, speed);
	l *= level;
	r *= level;

	if (l>1) l=1;
	else if (l<-1) l=-1;
	if (r>1) r=1;
	else if (l<-1) l=-1;

	left += l;
	right += r;
}

void DistortionGenerator::help(Help& help) const
{
	HelpEntry* entry=new HelpEntry("distortion","Distort sound");
	entry->addOption(new HelpOption("lvl","0..100, level of distorsion"));
	entry->addOption(new HelpOption("sound", "Sound generator to distort"));
	entry->addExample("distorsion 50 sinus 200");
	help.add(entry);
}

LevelSound::LevelSound(istream& in)
{
	in >> level;
}

void LevelSound::next(float& left, float& right, float speed)
{
	left += level;
	right += level;
}

FmGenerator::FmGenerator(istream& in) {
	a = 0;
	in >> min;
	in >> max;
	mod_gen = true;
	mod_mod = false;

	generator = factory(in);

	if (generator == 0) {
		if (last_type == "generator") {
			mod_gen = true;
			mod_mod = false;
		} else if (last_type == "modulator") {
			mod_gen = false;
			mod_mod = true;
		} else if (last_type == "both") {
			mod_gen = true;
			mod_mod = true;
		} else
			missingGeneratorExit("417");
	}

	if (generator == 0) generator = factory(in, true);
	modulator = factory(in, true);

	if (min < 0 || min > 2000 || max < 0 || max > 2000 || max < min) {
		// this->help(cerr); @TODO
		exit(1);
	}

	max /= 100.0;
	min /= 100.0;
}

void FmGenerator::next(float& left, float& right, float speed)
{
	if (min == max)
	{
		generator->next(left, right, mod_gen ? speed : 1.0);
		return;
	}

	float l=0,r=0;
	modulator->next(l, r, mod_mod ? speed : 1.0);	// nbre entre -1 et 1

	l = (l+r)/2.0;
	l = min + (max-min)*(l+1.0)/2.0;

	if (mod_gen) l *= speed;
	generator->next(left, right, l);
}

void FmGenerator::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("fm", "Frequency modulation");
	entry->addOption(new HelpOption("min", "0..200% modulation when modulator=-1"));
	entry->addOption(new HelpOption("max", "0..200% modulation when modulator=1"));
	entry->addOption(new HelpOption("spd_mod","[generator|modulator|both] : Where to apply speed modification ", true));
	entry->addOption(new HelpOption("sound", "What sound to modulate"));
	entry->addOption(new HelpOption("modulator", "Modulator of sound (any generator)"));
	entry->addExample("fm 80 120 sq 220 sin 10 : 220Hz square modulated with sinus");
	help.add(entry);
}

MixerGenerator::MixerGenerator(istream& in) {
	while (in.good()) {
		SoundGenerator* p = factory(in);
		if (p)
			generators.push_front(p);
		else
			break;
	}

	if (SoundGenerator::last_type != "}") {
		cerr << "Missing } at end of mixer generator" << endl;
		exit(1);
	}
}

void MixerGenerator::next(float& left, float& right, float speed)
{
	if (generators.size()==0)
		return;
	else if (generators.size()==1)
	{
		generators.front()->next(left, right, speed);
		return;
	}

	float l=0;
	float r=0;

	for(auto generator: generators)
		generator->next(l, r, speed);

	left += l / generators.size();
	right += r / generators.size();
}

void MixerGenerator::help(Help& help) const
{
	help.add(new HelpEntry("{ sound_1 [sound_2 ...] }", "mix together sounds and adjust volume accordingly"));
}

void LeftSound::next(float& left, float& right, float speed)
{
	float v=0;
	generator->next(left, v);
}

void LeftSound::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("left","Keep left part of signal");
	entry->addOption(new HelpOption("sound", "Sound generator to apply on"));
	help.add(entry);
}

RightSound::RightSound(istream& in) {
	generator = factory(in, true);
}

void RightSound::next(float& left, float& right, float speed)
{
	float v=0;
	generator->next(v, right);
}

void RightSound::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("right","Keep right part of signal");
	entry->addOption(new HelpOption("sound", "Generator to apply on"));
	help.add(entry);
}

EnvelopeSound::EnvelopeSound(istream& in) {
	int ms;
	index = 0;

	in >> ms;

	if (ms <= 0) {
		cerr << "Null duration" << endl;
		exit(1);
	}

	istream* input = 0;
	ifstream file;

	string type;
	in >> type;

	loop = true;
	if (type == "once") {
		loop = false;
		in >> type;
	} else if (type == "loop") {
		in >> type;
	}


	if (type == "data")
		input = &in;
	else if (type == "file") {
		string name;
		in >> name;
		file.open(name);
		input = &file;
	} else
		cerr << "Unkown type: " << type << endl;
	if (input == 0) {
		// this->help(cerr); @TODO
		exit(1);
	}
	float v;
	while (input->good()) {
		string s;
		(*input) >> s;
		if (s.length() == 0)
			break;
		if (s == "end")
			break;
		v = atof(s.c_str()) / 100.0;

		if (v<-200) v = -200;
		if (v > 200) v = 200;

		data.push_back(v);
	}
	generator = factory(in, true);

	dindex = 1000.0 * (data.size() - 1) / (float) ms / ech;
}

void EnvelopeSound::next(float& left, float& right, float speed)
{
	if (index>data.size())
		return;

	float l=0;
	float r=0;
	generator->next(l,r,speed);

	index += dindex;

	int idx=(int)index;
	float dec = index-idx;

	if (idx<0) idx=0;
	if (index>=(float)data.size()-1)
	{
		idx = data.size()-1;
		if (loop)
			index -= (float)data.size()-1;
	}
	float cur = data[idx];
	if (idx+1 < (int) data.size())
		idx++;
	float next = data[idx];

	float f = cur + (next-cur)*dec;

	left += l*f;
	right += r*f;
}

void EnvelopeSound::help(Help& help) const
{
	// @TODO
	/*HelpEntry* entry = new HelpEntry("envelope", "Linear enveloppe generator (time arguments)");
	entry->addOption(new HelpOption("ms:level","Time and level in millisecond and %"));
	entry->addExample("envelope 1000:100 1500:50");
	out << "envelope {timems} [once|loop] {data} generator" << endl;
	out << "  data is either : file filename, or data v1 ... v2 end" << endl;
	out << "  values are from -200 to 200 (float, >100 may distort sound)" << endl;
	 * */
}

MonoGenerator::MonoGenerator(istream& in) {
	generator = factory(in, true);
}

void MonoGenerator::next(float& left, float& right, float speed)
{
	float l=0;
	float v=0;
	generator->next(l,v,speed);
	l = (l+v)/2;
	left +=l;
	right +=l;
}

void MonoGenerator::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("mono","Mix left & right channel to monophonic output");
	entry->addOption(new HelpOption("sound", "Sound generator to transform"));
	help.add(entry);
}

AmGenerator::AmGenerator(istream& in) {
	in >> min;
	in >> max;

	if (in.good()) generator = factory(in, true);
	if (in.good()) modulator = factory(in, true);

	if (min < -200 || min > 200 || max < -200 || max > 200) {
		cerr << "Out of range (0..200)" << endl;
		exit(1);
	}

	max /= 100.0;
	min /= 100.0;
}

void AmGenerator::next(float& left, float& right, float speed)
{
	float l=0,r=0;
	generator->next(l, r, speed);

	float lv=0,rv=0;
	modulator->next(lv, rv, speed);

	lv = min + (max-min)*(lv+1)/2;
	rv = min + (max-min)*(rv+1)/2;

	left += lv*l;
	right += rv*r;
}

void AmGenerator::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("am", "Amplitude modulation");
	entry->addOption(new HelpOption("min", "-200..200 % level of amplification when modulator=-1"));
	entry->addOption(new HelpOption("max", "-200..200 % level of amplification when modulator=1"));
	entry->addOption(new HelpOption("sound", "A generator to modulate"));
	entry->addOption(new HelpOption("modulator", "A generator used as modulator"));
	entry->addExample("am 0 100 sq 220 sin 10 : 220Hz square modulated with sinus@10Hz");
	help.add(entry);
}

ReverbGenerator::ReverbGenerator(istream& in) {
	echo = (SoundGenerator::last_type == "echo");

	string s;

	in >> s;
	if (s.find(':') == string::npos) {
		cerr << "Missing :  in " << SoundGenerator::last_type << " generator." << endl;
		exit(1);
	}
	float ms = atof(s.c_str()) / 1000.0;
	s.erase(0, s.find(':') + 1);
	vol = atof(s.c_str()) / 100.0;

	if (ms <= 0) {
		cerr << "Negatif time not allowed." << endl;
		exit(1);
	}

	buf_size = ech * ms;

	if (buf_size == 0 || buf_size > 1000000) {
		cerr << "Bad Buffer size : " << buf_size << endl;
		exit(1);
	}

	buf_left = new float[buf_size];
	buf_right = new float[buf_size];
	for (uint32_t i = 0; i < buf_size; i++) {
		buf_left[i] = 0;
		buf_right[i] = 0;
	}
	index = 0;
	ech_vol = 1.0 - vol;
	ech_vol = 1.0;

	generator = factory(in, true);
}

void ReverbGenerator::next(float& left, float& right, float speed)
{
	float l=0;
	float r=0;
	generator->next(l,r,speed);

	if (echo)
	{
		float ll = buf_left[index];
		float rr = buf_right[index];

		buf_left[index] = l;
		buf_right[index] = r;

		l = l*ech_vol + ll*vol;
		r = r*ech_vol + rr*vol;
	}
	else
	{
		l = l*ech_vol + buf_left[index]*vol;
		r = r*ech_vol + buf_right[index]*vol;

		buf_left[index] = l;
		buf_right[index] = r;
	}
	index++;
	if (index==buf_size)
		index= 0;

	left += l;
	right += r;
}

void ReverbGenerator::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("reverb", "Reverberation");
	entry->addOption(new HelpOption("ms:vol", "Shift time (ms) and % level of reverb"));
	entry->addOption(new HelpOption("sound", "What sound to reverb (a generator)"));
	help.add(entry);
}

AdsrGenerator::AdsrGenerator(istream& in) {
	value v;
	value prev;
	prev.s = 0;
	prev.vol = 0;

	while (read(in, v)) {
		if (v <= prev) {
			cerr << "ADSR times must be ordered." << endl;
			cerr << "previous ms=" << prev.s << " current=" << v.s << endl;
			exit(1);
		}
		values.push_back(v);
		prev = v;
	}

	if (values.size() == 0) {
		cerr << "ADSR must contains at least 1 value at t>0ms." << endl;
		exit(1);
	}

	generator = factory(in, true);
	dt = 1.0 / (float) ech;
	restart();
}

void AdsrGenerator::restart()
{
	t=0;
	index = 0;
	target = values[0];
	previous.s = 0;
	previous.vol = 0;
}

bool AdsrGenerator::read(istream& in, value& val)
{
	string s;
	in >> s;
	if (s=="once")
	{
		loop = false;
		return false;
	}
	else if (s=="loop")
	{
		loop = true;
		return false;
	}

	if (s.find(':')==string::npos)
	{
		cerr << "Missing : in value or type (once/loop) for adsr." << endl;
		exit(1);
	}
	val.s = atof(s.c_str()) / 1000;
	s.erase(0, s.find(':')+1);
	val.vol = atof(s.c_str()) / 100;		
	return true;
}

void AdsrGenerator::next(float& left, float& right, float speed)
{
	float l=0;
	float r=0;
	generator->next(l, r, speed);

	if (index >= values.size())
	{
		left += l*target.vol;
		right += r*target.vol;
		return;
	}

	t += dt;
	while (t >= target.s)
	{
		previous = target;
		index++;
		if (index < values.size())
			target = values[index];
		else
		{
			if (loop) restart();
			break;
		}
	}

	float factor = (t-previous.s)  / (target.s - previous.s);
	float vol = previous.vol  + (target.vol-previous.vol)*factor;

	left += l*vol;
	right += r*vol;
}

void AdsrGenerator::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("adsr", "Attack Decay Sustain Release (Hold Delay etc) enveloppe generator");
	entry->addOption(new HelpOption("ms:vol", "Couples of time/level for the enveloppe (ms/%)"));
	entry->addOption(new HelpOption("...","next couples"));
	entry->addOption(new HelpOption("type", "'once' or 'loop' repeat option"));
	entry->addOption(new HelpOption("sound", "Sound generator to modify"));
	entry->addExample("adsr 1:0 1000:100 2000:0 loop sinus 440");
	help.add(entry);
}

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
		if (gen == 0)
			cerr << "Unknown generator " << type << endl;
	}
	if (gen == 0)
	{
		if (needed)
			missingGeneratorExit("");
	}
	return gen;
}

AvcRegulator::AvcRegulator(istream& in) {
	// in >> speed;
	speed = 0.999;
	min_gain = 0.05;

	generator = factory(in);

	reset();
}

void AvcRegulator::next(float& left, float& right, float sp)
{
	if (generator)
	{
		float l=0;
		float r=0;
		generator->next(l,r,sp);
		
		l *= gain;
		r *= gain;

		if (l>0.95 || l<-0.95 || r>0.95 || r<-0.95)
		{
			gain *= speed;
			cout << "GAIN : " << gain << " speed=" << speed << endl;
			if (gain < min_gain)
				gain = min_gain;
		}
		else if (gain < 1.0)
			gain *= 1.0001;

		left += l;
		right += r;
	}
}

void AvcRegulator::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("avc", "Automatic volume control");
	entry->addOption(new HelpOption("sound", "Affected sound generator"));
	help.add(entry);
}
#endif /* LIBSYNTH_HPP */

