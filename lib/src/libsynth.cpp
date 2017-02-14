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

uint32_t ech = 48000;

map<string, const SoundGenerator*> SoundGenerator::generators;
string					SoundGenerator::last_type;
map<string, string>		SoundGenerator::defines;
bool					SoundGenerator::echo=true;
bool					SoundGenerator::init_done = false;
SDL_AudioDeviceID		SoundGenerator::dev;
list<SoundGenerator*>	SoundGenerator::list_generator;
uint16_t				SoundGenerator::list_generator_size;	// avoid mx use
uint16_t				SoundGenerator::buf_size;
bool					SoundGenerator::saturate = false;
mutex					SoundGenerator::mtx;

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
static ChainSound gen_chain;
static Oscilloscope gen_osc;



TriangleGenerator::TriangleGenerator(istream& in)
{
	readFrequencyVolume(in);
	sign = 1;
	
	setValue("type", in);
	da = 4 / ((float) ech / freq);
	if (dir != BIDIR)
		da /= 2.0;
	reset();
}

bool TriangleGenerator::_setValue(string name, istream& in)
{
	dir = BIDIR;
	if (!in.good())
		return false;

	if (name == "type")
	{
		stringstream::pos_type last = in.tellg();
		string asc_desc;
		in >> asc_desc;
		
		if (asc_desc == "asc")
		{
			dir = ASC;
			sign = 1;
		}
		else if (asc_desc == "desc")
		{
			sign = -1;
			dir = DESC;
		}
		else
		{
			in.clear();
			in.seekg(last);
			return false;
		}
		return true;
	}
	else if (name=="f")
	{
		float f;
		in >> f;
		if (f <= 0)
			cerr << "Triangle : Invalid frequency" << endl;
		else
		{
			freq = f;
			da = 4 / ((float) ech / freq);
			return true;
		}
	}
	return false;
}

void TriangleGenerator::reset()
{
	if (dir == ASC)
		a = -1;
	else if (dir == DESC)
		a = 1;
	else
		a = 0;
}

void TriangleGenerator::next(float& left, float& right, float speed)
{
	a += da * speed * (float)sign;
	
	float dv=a;
	
	if (dv>1)
	{
		dv=1;
		if (dir == ASC)
			reset();
		else
			sign = -sign;
	}
	else if (dv<-1)
	{
		dv=-1;
		if (dir == DESC)
			reset();
		else
			sign = -sign;
	}
	left += (float)dv * volume;
	right += (float)dv * volume;
}

void TriangleGenerator::help(Help& help) const
{
	HelpEntry* entry = SoundGenerator::addHelpOption(new HelpEntry("triangle","triangle sound"));
	entry->addOption(new HelpOption("type", "[tri|asc|desc] Type of signal triangle or asc/desc sawtooth (default  :tri)", HelpOption::OPTIONAL | HelpOption::CHOICE));
	help.add(entry);
}


SquareGenerator::SquareGenerator(istream& in)
{
	readFrequencyVolume(in);
	a = 0;
	da = 1.0f / (float) ech;
	val = 1;
}

bool SquareGenerator::_setValue(string name, istream& in)
{
	if (name != "v" && name != "f")
		return false;
	
	if (name=="f")
		invert = (float) (ech >> 1) / freq;
	
	return true;
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
{
	readFrequencyVolume(in);
	a = 0;
}

bool SinusGenerator::_setValue(string name, istream& in)
{
	if (name=="f")
	{
		in >> freq;
		da = (2 * M_PI * freq) / (float) ech;
		return true;
	}
	return false;
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

DistortionGenerator::DistortionGenerator(istream& in)
{
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
	entry->addOption(new HelpOption("sound", "Sound generator to distort", HelpOption::GENERATOR));
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
	entry->addOption(new HelpOption("spd_mod","[generator|modulator|both] : Where to apply speed modification ",  HelpOption::OPTIONAL | HelpOption::CHOICE));
	entry->addOption(new HelpOption("sound", "What sound to modulate", HelpOption::GENERATOR));
	entry->addOption(new HelpOption("modulator", "Modulator of sound (any generator)", HelpOption::GENERATOR));
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
	HelpEntry* entry = new HelpEntry("{ $ }", "mix together sounds and adjust volume accordingly");
	entry->addOption(new HelpOption("sound", "Sound generators", HelpOption::OPTIONAL | HelpOption::REPEAT | HelpOption::GENERATOR));
	help.add(entry);
}

void LeftSound::next(float& left, float& right, float speed)
{
	float v=0;
	generator->next(left, v);
}

void LeftSound::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("left","Keep left part of signal");
	entry->addOption(new HelpOption("sound", "Sound generator to apply on", HelpOption::GENERATOR));
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
	entry->addOption(new HelpOption("sound", "Generator to apply on", HelpOption::GENERATOR));
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
	entry->addOption(new HelpOption("sound", "Sound generator to transform", HelpOption::GENERATOR));
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
	entry->addOption(new HelpOption("sound", "A generator to modulate", HelpOption::GENERATOR));
	entry->addOption(new HelpOption("modulator", "A generator used as modulator", HelpOption::GENERATOR));
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
	entry->addOption(new HelpOption("ms:vol", "Shift time (ms) and % level of reverb", HelpOption::MS_VOL));
	entry->addOption(new HelpOption("sound", "What sound to reverb (a generator)", HelpOption::GENERATOR));
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
	reset();
}

void AdsrGenerator::reset()
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
	if (generator==0)
		return;
	
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
			if (loop) reset();
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
	entry->addOption(new HelpOption("ms:vol", "Couples of time/level for the enveloppe (ms/%)", HelpOption::REPEAT | HelpOption::MS_VOL));
	entry->addOption(new HelpOption("type", "[once|loop] repeat option", HelpOption::CHOICE));
	entry->addOption(new HelpOption("sound", "Sound generator to modify", HelpOption::GENERATOR));
	entry->addExample("adsr 1:0 1000:100 2000:0 loop sinus 440");
	help.add(entry);
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
	entry->addOption(new HelpOption("sound", "Affected sound generator", HelpOption::GENERATOR));
	help.add(entry);
}

void ChainSound::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("chain", "Chain sounds in sequence");
	entry->addOption(new HelpOption("adsr", "Sounds will be played with an adsr", HelpOption::GENERATOR));
	entry->addOption(new HelpOption("ms xxx", "Set default duration (can appear many times"));
	entry->addOption(new HelpOption("gaps ms", "Gaps between sounds (can appear many times)",  HelpOption::OPTIONAL));
	entry->addOption(new HelpOption("[ms] sound", "Duration /  Sound generator", HelpOption::REPEAT));
	entry->addOption(new HelpOption("end", "end of sequence"));
	help.add(entry);
}

ChainSound::ChainSound(istream& in)
{
	adsr = 0;
	uint32_t ms=0;
	uint32_t def_ms=0;
	while(in.good())
	{
		string sms;
		stringstream::pos_type last = in.tellg();
		
		in >> sms;
		if (sms == "end")
			break;
		
		if (sms == "ms")
		{
			in >> def_ms;
			if (def_ms <= 0)
			{
				cerr << "Cannot have null default duration" << endl;
				exit(1);
			}
		}
		else if (sms == "gaps")
			in >> gaps;
		else if (sms == "adsr")
		{
			if (adsr == 0)
				adsr = new AdsrGenerator(in);
			else
			{
				cerr << "Cannot have multiple adsr" << endl;
				exit(1);
			}
		}
		else
		{
			uint32_t delta = atol(sms.c_str());
			if (delta == 0)
			{
				in.clear();
				in.seekg(last);
				delta = def_ms;
				if (delta == 0)
				{
					cerr << "No default duration (or missing duration)" << endl;
					exit(1);
				}
			}
			ms += delta;
			
			SoundGenerator* sound = factory(in);
			if (sound)
				add(ms, sound);
			else
				SoundGenerator::missingGeneratorExit();
				
			if (gaps)
			{
				ms += gaps;
				sounds.push_back(ChainElement(ms, 0));
			}
		}
	}
	dt = 1.0/(float)ech;
	reset();
}

void ChainSound::add(uint32_t ms, SoundGenerator* g)
{
	if (g)
		sounds.push_back(ChainElement(ms, g));
}

void ChainSound::reset()
{
	t = 0;
	it = sounds.begin();
	if (adsr && it != sounds.end())
		adsr->setSound(it->sound);
}

void ChainSound::next(float& left, float& right, float speed)
{
	if (it!=sounds.end())
	{
		t += dt;
		ChainElement& sound=*it;
		if (t>sound.t)
		{
			it++;
			if (adsr)
			{
				adsr->reset();
				adsr->setSound(it->sound);
			}
		}
		if (sound.sound)
		{
			if (adsr)
				adsr->next(left, right, speed);
			else
				sound.sound->next(left, right, speed);
		}
	}
}


#endif /* LIBSYNTH_HPP */

