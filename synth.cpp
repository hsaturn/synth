#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <list>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>

using namespace std;

uint32_t ech = 48000;

const int BUF_LENGTH = 8192;

class SoundGenerator
{
	public:
		virtual ~SoundGenerator() {};

		// Next sample to add to left and right 
		// added value should be from -1 to 1
		// speed = samples/sec modifier
		virtual void next(float &left, float &right, float speed=1.0) = 0;

		static SoundGenerator* factory(istream& in, bool needed=false);
		static string getTypes()
		{
			string s;
			for(auto it: generators)
				s+= it.first+' ';
			return s;
		}

		static float rand()
		{
			return (float)::rand() / ((float)RAND_MAX/2) -1.0;
		}

		static void help()
		{
			map<const SoundGenerator*, bool>	done;
			for(auto generator : generators)
			{
				if (done.find(generator.second)==done.end())
				{
					done[generator.second] = true;
					generator.second->help(cout);
				}
			}
		}
	
	protected:
		SoundGenerator() {};

		// Auto register for the factory
		SoundGenerator(string name)
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

		// Generic constructor for freq:vol
		SoundGenerator(istream& in)
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

			if (volume<0 || volume>1)
			{
				cerr << "Invalid volume " << volume*100 << endl;
				exit(1);
			}
		};

		virtual SoundGenerator* build(istream& in) const=0;
		virtual void help(ostream& out) const
		{
			out << "freq[:volume]";
		}

		float volume;
		float freq;

		static string last_type;
	private:
		static map<string, const SoundGenerator*> generators;

		// For the factory
};

string SoundGenerator::last_type;
map<string, const SoundGenerator*> SoundGenerator::generators;

class WhiteNoiseGenerator : public SoundGenerator
{
	public:
		WhiteNoiseGenerator() : SoundGenerator("wnoise"){}	// factory

		WhiteNoiseGenerator(istream& in)
		{
		};

		virtual void next(float &left, float &right, float speed=1.0)
		{
			left += SoundGenerator::rand();
			right += SoundGenerator::rand();
		}

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new WhiteNoiseGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "wnoise" << endl;
		}

};

class TriangleGenerator : public SoundGenerator
{
	public:
		TriangleGenerator() : SoundGenerator("tri triangle"){};	// factory

		TriangleGenerator(istream& in)
		:
		SoundGenerator(in)
		{
			a = 0;
			da = 4 / ((float)ech / freq);
			sign = 1;
			cout << "TRIANGLE FREQUENCY : " << freq << ' ' << da << endl;
		}

		virtual void next(float &left, float &right, float speed=1.0)
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

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new TriangleGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "triangle ";
			SoundGenerator::help(out);
			out << endl;
		}

	private:
		float a;
		float da;
		int sign;
};


class SquareGenerator : public SoundGenerator
{
	public:
		SquareGenerator() : SoundGenerator("sq square") {}

		SquareGenerator(istream& in)
		: SoundGenerator(in)
		{
			a = 0;
			da = 1.0f / (float)ech;
			val=1;
			invert = (float)(ech>>1) / freq;
		}

		virtual void next(float &left, float &right, float speed=1.0)
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

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new SquareGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "square ";
			SoundGenerator::help(out);
			out << endl;
		}

	private:
		float a;
		float da;
		float invert;
		int val;
};

class SinusGenerator : public SoundGenerator
{
	public:
		SinusGenerator() : SoundGenerator("sin sinus") {};

		SinusGenerator(istream& in)
		: SoundGenerator(in)
		{
			a = 0;
			da = (2 * M_PI * freq) / (float)ech;
			cout << "SINUS FREQUENCY : " << freq << endl;
		}

		virtual void next(float &left, float &right, float speed=1.0)
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

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new SinusGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "sinus ";
			SoundGenerator::help(out);
			out << endl;
		}

	private:
		float a;
		float da;
};

class DistortionGenerator : public SoundGenerator
{
	public:
		DistortionGenerator() : SoundGenerator("distorsion"){}

		DistortionGenerator(istream& in)
		{
			in >> level;
			if (in.good()) generator = SoundGenerator::factory(in, true);
			if (level<0 || level>100)
			{
				this->help(cerr);
				exit(1);
			}

			level = level/100+1.0f;
		}
		
		virtual void next(float &left, float &right, float speed)
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

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new DistortionGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "distorsion level generator" << endl;
			out << "  level = 0..100, ";
			out << "  example: distorsion 50 sinus 200" << endl;
		}

	private:
		float level;
		SoundGenerator* generator;
};

class FmGenerator : public SoundGenerator
{
	public:
		FmGenerator() : SoundGenerator("fm") {}	// for thefactory

		FmGenerator(istream& in)
		{
			a = 0;
			in >> min;
			in >> max;

			if (in.good()) generator = SoundGenerator::factory(in, true);
			if (in.good()) modulator = SoundGenerator::factory(in, true);

			if (min<0 || min>200 || max<0 || max>200 || max<min)
			{
				this->help(cerr);
				exit(1);
			}

			max /= 100.0;
			min /= 100.0;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (min == max)
			{
				generator->next(left, right, speed);
				return;
			}

			float l=0,r=0;
			modulator->next(l, r);	// nbre entre -1 et 1

			l = (l+r)/2.0;
			l = min + (max-min)*(l+1.0)/2.0;

			generator->next(left, right, l);
		}
		
	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new FmGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "fm min max {sound_generator} {sound_modulator}" << endl;
			out << "  min : minimum frequency shifting level" << endl;
			out << "  max : max frequency shifting (0..200)" << endl;
			out << "  sound_generator : a sound generator (as usual)" << endl;
			out << "  sound_modulator : the frequency modulator (another sound generator)" << endl;
			out << "  ex: fm 80 120 sq 220 sin 10 : 220Hz square modulated with sinus" << endl;
			out << endl;
		}

	private:
		float min;
		float max;
		float a;
		SoundGenerator* generator;
		SoundGenerator* modulator;
		float last_ech_left;
		float last_ech_right;
};

class MixerGenerator : public SoundGenerator
{
	public:
		MixerGenerator() : SoundGenerator("{"){};

		MixerGenerator(istream& in)
		{
			while (in.good())
			{
				SoundGenerator* p=SoundGenerator::factory(in);
				if (p)
					generators.push_front(p);
				else
					break;
			}

			if (SoundGenerator::last_type != "}")
			{
				cerr << "Missing } at end of mixer generator" << endl;
				exit(1);
			}
			cout << "Mixer size : " << generators.size() << endl;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (generators.size()==0)
				return;

			float l=0;
			float r=0;

			for(auto generator: generators)
				generator->next(l, r, speed);

			left += l / generators.size();
			right += r / generators.size();
		}

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new MixerGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "{ generator [generator ...] }" << endl;
		}
	
	private:
		list<SoundGenerator*>	generators;

};

class LeftSound : public SoundGenerator
{
	public:
		LeftSound() : SoundGenerator("left") {}

		LeftSound(istream& in)
		{
			generator = SoundGenerator::factory(in, true);
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			float v;
			generator->next(left, v);
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new LeftSound(in); }

		virtual void help(ostream& out) const
		{
			out << "left generator : keep only the left channel" << endl;
		}

	private:
		SoundGenerator* generator;
};

class RightSound : public SoundGenerator
{
	public:
		RightSound() : SoundGenerator("right") {}

		RightSound(istream& in)
		{
			generator = SoundGenerator::factory(in, true);
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			float v;
			generator->next(v, right);
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new RightSound(in); }

		virtual void help(ostream& out) const
		{
			out << "right generator : keep only the right channel" << endl;
		}

	private:
		SoundGenerator* generator;
};

class EnvelopeSound : public SoundGenerator
{
	public:
		EnvelopeSound() : SoundGenerator("envelope env") {}

		EnvelopeSound(istream &in)
		{
			int ms;
			index = 0;

			in >> ms;

			if (ms<=0)
			{
				cerr << "Null duration" << endl;
				exit(1);
			}

			istream* input = 0;
			ifstream file;

			string type;
			in >> type;

			loop=true;
			if (type=="once")
			{
				loop=false;
				in >> type;
			}
			else if (type=="loop")
			{
				in >> type;
			}


			if (type=="data")
				input = &in;
			else if (type=="file")
			{
				string name;
				in >> name;
				file.open(name);
				input = &file;
				cout << "From file " << name << endl;
			}
			else
				cerr << "Unkown type: " << type << endl;
			if (input == 0)
			{
				this->help(cerr);
				exit(1);
			}
			float v;
			while(input->good())
			{
				string s;
				(*input) >> s;
				if (s.length()==0)
					break;
				if (s=="end")
					break;
				v = atof(s.c_str())/100.0;

				if (v<200) v=-200;
				if (v>200) v=200;

				data.push_back(v);
			}
			cout << "data ";
			for(auto v: data)
				cout << v << ' ';
			generator = SoundGenerator::factory(in, true);

			dindex =  1000.0 * (data.size()-1) / (float)ms / ech;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			if (index>data.size())
				return;

			float l;
			float r;
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

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new EnvelopeSound(in); }

		virtual void help(ostream& out) const
		{
			out << "envelope {timems} [once|loop] {data} generator" << endl;
			out << "  data is either : file filename, or data v1 ... v2 end" << endl;
			out << "  values are from -200 to 200 (float, >100 may distort sound)" << endl;
		}

	private:
		bool loop;
		float index;
		float dindex;

		vector<float>	data;
		SoundGenerator* generator;
};

class AmGenerator : public SoundGenerator
{
	public:
		AmGenerator() : SoundGenerator("am"){};	// for the factory

		AmGenerator(istream &in)
		{
			in >> min;
			in >> max;

			if (in.good()) generator = SoundGenerator::factory(in, true);
			if (in.good()) modulator = SoundGenerator::factory(in, true);

			if (min<0 || min>100 || max<0 || max>100)
			{
				this->help(cerr);
				exit(1);
			}

			max /= 100.0;
			min /= 100.0;
		}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			float l=0,r=0;
			generator->next(l, r, speed);

			float lv=0,rv=0;
			modulator->next(lv, rv, speed);

			lv = min + (max-min)*(lv+1)/2;
			rv = min + (max-min)*(rv+1)/2;

			left += lv*l;
			right += lv*r;
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new AmGenerator(in); }

		virtual void help(ostream& out) const
		{
			out << "am min max {sound_generator} {sound_modulator}" << endl;
			out << "  min : minimum sound level" << endl;
			out << "  max : max sound level (0..100)" << endl;
			out << "  sound_generator : a sound generator (as usual)" << endl;
			out << "  sound_modulator : the amplitude modulator (another sound generator)" << endl;
			out << "  ex: am 0 100 sq 220 sin 10 : 220Hz square modulated with sinus" << endl;
			out << endl;
		}

	private:
		float min;
		float max;
		SoundGenerator* generator;
		SoundGenerator* modulator;
};

SoundGenerator* SoundGenerator::factory(istream& in, bool needed)
{
	string type;
	in >> type;
	cout << "GENERATOR TYPE " << type << endl;
	auto it = generators.find(type);
	if (it == generators.end())
	{
		if (needed)
		{
			cerr << "Missing generator" << endl;
			exit(1);
		}
		last_type = type;
		return 0;
	}
	return it->second->build(in);
}

list<SoundGenerator*>	list_generator;

void help()
{
	cout << "Syntax : " << endl;
	cout << "  synth [duration] [sample_freq] generator_1 [generator_2 [...]]" << endl;
	cout << endl;
	cout << "  duration     : sound duration (ms)" << endl;
	cout << endl;
	cout << "Available sound generators : " << endl;

	SoundGenerator::help();

	exit(1);
}

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


void audioCallback(void *unused, Uint8 *byteStream, int byteStreamLength) {
	if (list_generator.size()==0)
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

		stream[i] = 32767 * left / list_generator.size();
		stream[i+1] = 32767 * right / list_generator.size();
	}

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

	ech = atol(argv[i]);
	if (ech==0)
		ech=48000;
	else
		i++;

	for(; i<argc; i++)
	{
		if ((string)argv[i]=="help" || (string)argv[i]=="-h")
			help();
		input << argv[i] << ' ';
	}

	string arg;
	while(input.good())
	{
		SoundGenerator* g = SoundGenerator::factory(input);
		if (g)
			list_generator.push_front(g);
	}

	if (list_generator.size()==0)
		help();

	cout << "Number of generators : " << list_generator.size() << ", now playing ..." << endl;

	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER);
	SDL_AudioSpec want, have;
	SDL_AudioDeviceID dev;

	SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
	want.freq = ech;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = BUF_LENGTH;
	want.callback = audioCallback;

	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (dev == 0) {
		SDL_Log("Failed to open audio: %s", SDL_GetError());
	} else {
		if (have.format != want.format) { /* we let this one thing change. */
			SDL_Log("We didn't get Float32 audio format.");
		}
		cout << "ECH = " << have.freq << " duration=" << duration << endl;
		SDL_PauseAudioDevice(dev, 0); /* start audio playing. */
		SDL_Delay(duration); // Play for ms
		SDL_CloseAudioDevice(dev);
		SDL_Quit();
	}

	return 0;
}
