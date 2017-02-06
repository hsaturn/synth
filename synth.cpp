#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <list>
#include <sstream>
#include <map>

using namespace std;

const int ECH = 48000;
const int BUF_LENGTH = 1024;

class SoundGenerator
{
	public:
		static const int ech = ECH;



		virtual ~SoundGenerator() {};
		virtual void next(float &left, float &right) = 0;

		static SoundGenerator* factory(istream& in);
		static string getTypes()
		{
			string s;
			for(auto it: generators)
				s+= it.first+' ';
			return s;
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

		float volume;
		float freq;

	private:
		static map<string, const SoundGenerator*> generators;

		// For the factory
};

map<string, const SoundGenerator*> SoundGenerator::generators;

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

		virtual void next(float &left, float &right)
		{
			a += da * (float)sign;
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

		virtual void next(float &left, float &right)
		{
			a ++;
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

		virtual void next(float &left, float &right)
		{
			static int count=0;
			count++;
			a += da;
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
			if (in.good()) generator = SoundGenerator::factory(in);
			if (level<0 || level>100 || generator == 0)
			{
				cerr << "distorsion syntax: distorsion leve250Gl generator" << endl;
				cerr << "level = 0..100, generator= sound generator" << endl;
				cerr << "example: distorsion 50 sinus 200" << endl;
				exit(1);
			}

			level = level/100+1.0f;
		}
		
		virtual void next(float &left, float &right)
		{
			float l,r;
			generator->next(l,r);
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

	private:
		float level;
		SoundGenerator* generator;
};

class AmGenerator : public SoundGenerator
{
	public:
		AmGenerator() : SoundGenerator("am"){};	// for the factory

		AmGenerator(istream& in)
		{
			in >> min;
			in >> max;

			if (in.good()) generator = SoundGenerator::factory(in);
			if (in.good()) modulator = SoundGenerator::factory(in);

			if (min<0 || min>100 || max<0 || max>100 || modulator == 0 || generator == 0)
			{
				cerr << "am generator syntax is :" << endl;
				cerr << "  am min max {sound_generator} {sound_modulator}" << endl;
				cerr << "  min : minimum sound level" << endl;
				cerr << "  max : max sound level (0..100)" << endl;
				cerr << "  sound_generator : a sound generator (as usual)" << endl;
				cerr << "  sound_modulator : the amplitude modulator (another sound generator)" << endl;
				cerr << "  ex: am 0 100 sq 220 sin 10 : 220Hz square modulated with sinus" << endl;
				cerr << endl;
				cerr << "yet defined values where min:" << min << " max:" << max << " generator:" << generator << " modulator:" << modulator << endl;
				exit(1);
			}

			max /= 100.0;
			min /= 100.0;
		}

		virtual void next(float &left, float &right)
		{
			float l=0,r=0;
			generator->next(l, r);

			float lv=0,rv=0;
			modulator->next(lv, rv);

			lv = min + (max-min)*(lv+1)/2;
			rv = min + (max-min)*(rv+1)/2;

			left += lv*l;
			right += lv*r;
		}

	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new AmGenerator(in); }

	private:
		float min;
		float max;
		SoundGenerator* generator;
		SoundGenerator* modulator;
};

SoundGenerator* SoundGenerator::factory(istream& in)
{
	string type;
	in >> type;
	auto it = generators.find(type);
	if (it == generators.end())
	{
		cerr << "Unknown generator : (" << type << ')' << endl;
		exit(1);
	}
	return it->second->build(in);
}

list<SoundGenerator*>	list_generator;

void audioCallback(void *unused, Uint8 *byteStream, int byteStreamLength) {
	if (list_generator.size()==0)
		return;
	int ech = byteStreamLength / sizeof(int16_t);
	int16_t* stream =  reinterpret_cast<int16_t*>( byteStream );
	Uint32 i;

	for (i = 0; i < ech; i)
	{
		float left = 0;
		float right = 0;
		for(auto generator: list_generator)
			generator->next(left, right);

		stream[i] = 32767 * left / list_generator.size();
		stream[i+1] = 32767 * right / list_generator.size();

		i+=2;
	}

}

void help()
{
	cout << "Syntax : " << endl;
	cout << "  gen [duration] generator_1 [generator_2 [...]]" << endl;
	cout << endl;
	cout << "  duration     : sound duration (ms)" << endl;
	cout << "  generator is : type freq[:vol]" << endl;
	cout << "  type = " << SoundGenerator::getTypes() << endl;
	cout << "  vol  = 0..100 (percent)" << endl;
	cout << endl;
	exit(1);
}

// Auto register for the factory
static SquareGenerator gen_sq;
static TriangleGenerator gen_tr;
static SinusGenerator gen_si;
static AmGenerator gen_am;
static DistortionGenerator gen_dist;

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
		if ((string)argv[i]=="help" || (string)argv[i]=="-h")
			help();
		input << ' ' << argv[i];
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
	want.freq = ECH;
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
		SDL_PauseAudioDevice(dev, 0); /* start audio playing. */
		SDL_Delay(duration); // Play for ms
		SDL_CloseAudioDevice(dev);
		SDL_Quit();
	}

	return 0;
}
