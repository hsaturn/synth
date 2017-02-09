#ifndef LIBSYNTHETISER
#define LIBSYNTHETISER

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <list>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <memory>

using namespace std;

const int BUF_LENGTH = 8192;

class SoundGenerator
{
	public:
		static bool init(uint16_t wanted_buf_size=2048);
		
		static void quit();
		
		virtual ~SoundGenerator() {};

		// Next sample to add to left and right 
		// added value should be from -1 to 1
		// speed = samples/sec modifier
		virtual void next(float &left, float &right, float speed=1.0) = 0;
		
		virtual void reset() {};

		static SoundGenerator* factory(istream& in, bool needed=false);
		static string getTypes();

		/**
		 * @return -1..1 float
		 */
		static float rand();

		static void help();

		static string last_type;

		static void missingGeneratorExit(string msg="");
		
		static void play(SoundGenerator*);
		static void stop(SoundGenerator*);
		
		// Return the number of active playing generators.
		static uint16_t count(){ return list_generator_size; }
		
		static uint16_t bufSize() { return buf_size; }

		class HelpOption
		{
			public:
				HelpOption(string arg_name, string arg_desc, bool optional=false) : name(arg_name), desc(arg_desc), option(optional){};
				
				const string& getName() const { return name; }
				const string& getDesc() const { return desc; }
				bool isOptional() const { return option; }
				
			private:
				string name;
				string desc;
				bool option;
		};
		class HelpEntry
		{
			public:
				HelpEntry(string command, string description) : cmd(command), desc(description) {}
				void addOption(HelpOption* option) { options.push_back(shared_ptr<HelpOption>(option)); }
				void addExample(string ex) { example = ex; }
				string concatOptions() const;
				const string& getCmd() const { return cmd; }
				const string& getDesc() const { return desc; }
				const string& getExample() const { return example; }
				const list<shared_ptr<HelpOption>>& getOptions() const { return options; }
				
			private:
				string cmd;
				string desc;
				string example;
				list<shared_ptr<HelpOption>> options;
		};
		
		class Help
		{
			public:
				void add(HelpEntry* entry) { entries.push_back(shared_ptr<HelpEntry>(entry)); }
				friend class SoundGenerator;
				
				friend ostream& operator<<(ostream& out, const Help&);
				Help(){};
				
				static string padString(string s, string::size_type length);
				
			private:
				
				string cmd;
				string desc;
				list<shared_ptr<HelpEntry>> entries;
		};
	protected:
		SoundGenerator() {};

		// Auto register for the factory
		SoundGenerator(string name);

		// Generic constructor for freq:vol
		SoundGenerator(istream& in);


		virtual SoundGenerator* build(istream& in) const=0;
		virtual void help(Help& help) const;
		void help(ostream&) const;
		HelpEntry* addHelpOption(HelpEntry*) const;
		
		// Main audio callback
		static void audioCallback(void *unused, Uint8 *byteStream, int byteStreamLength);

		float volume;
		float freq;
		
		static void close();
		
		/**
		 * @return bool saturation has occured (reseted) 
		 */
		static bool saturated()
		{
			if (saturate)
			{
				saturate = false;
				return true;
			}
			return false;
		}

	private:
		static map<string, const SoundGenerator*> generators;
		static map<string, string> defines;
		static bool echo;
		static bool init_done;
		static 	SDL_AudioDeviceID dev;
		static list<SoundGenerator*>	list_generator;
		static uint16_t list_generator_size;
		static mutex mtx;
		static uint16_t buf_size;
		static bool saturate;
};

template<class T>
class SoundGeneratorVarHook : public SoundGenerator
{
	public:
		SoundGeneratorVarHook(atomic<T>* v, T min, T max, string name)
		:
		mref(v),
		mmin(min),
		mmax(max),
		SoundGenerator(name){}

		SoundGeneratorVarHook(istream &in, atomic<T>* v, T min, T max)
		:
		mref(v), mmin(min), mmax(max) {}

		virtual void next(float &left, float &right, float speed=1.0)
		{
			float delta = (float)(*mref - mmin)/(float)(mmax-mmin);

			left += delta;
			right += delta;
		}

		virtual void help(ostream &out) const
		{ out << "Help not defined (SoundGeneratorVarHook)" << endl; }

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new SoundGeneratorVarHook<T>(in, mref,mmin, mmax); }

	private:
		atomic<T>* mref;
		T mmin;
		T mmax;
};

class WhiteNoiseGenerator : public SoundGenerator
{
	public:
		WhiteNoiseGenerator() : SoundGenerator("wnoise"){}	// factory

		WhiteNoiseGenerator(istream& in) {};

		virtual void next(float &left, float &right, float speed=1.0)
		{
			left += SoundGenerator::rand();
			right += SoundGenerator::rand();
		}

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new WhiteNoiseGenerator(in); }

		virtual void help(Help& help) const
		{	help.add(new HelpEntry("wnoise","Generator white noise")); }

};

class TriangleGenerator : public SoundGenerator
{
	public:
		TriangleGenerator() : SoundGenerator("tri triangle") {};	// factory

		TriangleGenerator(istream& in);

		virtual void next(float &left, float &right, float speed=1.0);


	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new TriangleGenerator(in); }

		virtual void help(Help& help) const;

	private:
		float a;
		float da;
		int sign;
};


class SquareGenerator : public SoundGenerator
{
	public:
		SquareGenerator() : SoundGenerator("sq square") {};

		SquareGenerator(istream& in);

		virtual void next(float &left, float &right, float speed=1.0);


	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new SquareGenerator(in); }

		virtual void help(Help& help) const;

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

		SinusGenerator(istream& in);

		virtual void next(float &left, float &right, float speed=1.0);


	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new SinusGenerator(in); }

		virtual void help(Help& help) const;


	private:
		float a;
		float da;
};

class DistortionGenerator : public SoundGenerator
{
	public:
		DistortionGenerator() : SoundGenerator("distorsion"){}

		DistortionGenerator(istream& in);
		
		virtual void next(float &left, float &right, float speed);


	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new DistortionGenerator(in); }

		virtual void help(Help& help) const;

	private:
		float level;
		SoundGenerator* generator;
};

class LevelSound : public SoundGenerator
{
	public:
		LevelSound() : SoundGenerator("level"){}

		LevelSound(istream &in);

		virtual void next(float &left, float &right, float speed=0.1);


		virtual void help(ostream &out) const
		{ out << "level value : constant level" << endl; }

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new LevelSound(in); }

	private:
		float level;
};

class FmGenerator : public SoundGenerator
{
	public:
		FmGenerator() : SoundGenerator("fm") {}	// for thefactory

		FmGenerator(istream& in);

		virtual void next(float &left, float &right, float speed=1.0);

		
	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new FmGenerator(in); }

		virtual void help(Help& help) const;

	private:
		float min;
		float max;
		float a;
		SoundGenerator* generator;
		SoundGenerator* modulator;
		float last_ech_left;
		float last_ech_right;
		bool mod_gen;
		bool mod_mod;
};

class MixerGenerator : public SoundGenerator
{
	public:
		MixerGenerator() : SoundGenerator("{"){};

		MixerGenerator(istream& in);

		virtual void next(float &left, float &right, float speed=1.0);


	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new MixerGenerator(in); }

		virtual void help(Help& help) const;

	private:
		list<SoundGenerator*>	generators;

};

class LeftSound : public SoundGenerator
{
	public:
		LeftSound() : SoundGenerator("left") {}

		LeftSound(istream& in)
		{
			generator = factory(in, true);
		}

		virtual void next(float &left, float &right, float speed=1.0);


	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new LeftSound(in); }

		virtual void help(Help& help) const;


	private:
		SoundGenerator* generator;
};

class RightSound : public SoundGenerator
{
	public:
		RightSound() : SoundGenerator("right") {}

		RightSound(istream& in);

		virtual void next(float &left, float &right, float speed=1.0);


	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new RightSound(in); }

		virtual void help(Help& help) const;

	private:
		SoundGenerator* generator;
};

class EnvelopeSound : public SoundGenerator
{
	public:
		EnvelopeSound() : SoundGenerator("envelope env") {}

		EnvelopeSound(istream &in);

		virtual void next(float &left, float &right, float speed=1.0);


	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new EnvelopeSound(in); }

		virtual void help(Help& help) const;

	private:
		bool loop;
		float index;
		float dindex;

		vector<float>	data;
		SoundGenerator* generator;
};

class MonoGenerator : public SoundGenerator
{
	public:
		MonoGenerator() : SoundGenerator("mono") {}

		MonoGenerator(istream &in);

		virtual void next(float &left, float &right, float speed = 1.0);

		virtual void help(Help& help) const;

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new MonoGenerator(in); }

	private:
		SoundGenerator* generator;
};

class AmGenerator : public SoundGenerator
{
	public:
		AmGenerator() : SoundGenerator("am"){};	// for the factory

		AmGenerator(istream &in);

		virtual void next(float &left, float &right, float speed=1.0);
	protected:
		virtual SoundGenerator* build(istream& in) const
		{ return new AmGenerator(in); }

		virtual void help(Help& help) const;

	private:
		float min;
		float max;
		SoundGenerator* generator;
		SoundGenerator* modulator;
};

class ReverbGenerator : public SoundGenerator
{
	public:
		ReverbGenerator() : SoundGenerator("reverb echo") {}

		ReverbGenerator(istream& in);

		virtual void next(float &left, float &right, float speed=1.0);

	protected:
		virtual void help(Help& help) const;

		virtual SoundGenerator* build(istream &in) const
		{ return new ReverbGenerator(in); }

	private:
		bool echo;
		float vol;
		float ech_vol;
		float* buf_left;
		float* buf_right;
		uint32_t	buf_size;
		uint32_t	index;
		SoundGenerator* generator;
};

class AvcRegulator : public SoundGenerator
{
	public:
		AvcRegulator() : SoundGenerator("avc"){};
		
		AvcRegulator(istream &in);
		
		virtual void reset() { gain = 1.0; }
		
		virtual void next(float &left, float &right, float speed=1.0);
		
		virtual void help(Help& help) const;
		
	private:
		virtual SoundGenerator* build(istream& in) const
		{ return new AvcRegulator(in); }
		
		SoundGenerator* generator;
		float speed;
		float gain;
		float min_gain;
};

class AdsrGenerator : public SoundGenerator
{
	struct value
	{
		float s;
		float vol;

		bool operator <= (const value &v) { return s <= v.s; }

		friend ostream& operator<< (ostream &out, const value &v)
		{
			out << '(' << v.s << "s, " << v.vol << ')';
			return out;
		}
	};

	public:
		AdsrGenerator() : SoundGenerator("adsr"){}

		AdsrGenerator(istream& in);

		virtual void reset();

		bool read(istream &in, value &val);

		virtual void next(float &left, float &right, float speed=1.0);
		
		void setSound(SoundGenerator* sound) { generator = sound; }

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new AdsrGenerator(in); }

		virtual void help(Help &) const;

	private:
		float t;
		float dt;

		value previous;
		value target;
		uint32_t index;
		vector<value>	values;
		SoundGenerator* generator;
		bool loop;
};

class ChainSound : public SoundGenerator
{
	class ChainElement
	{
		public:
			ChainElement(uint32_t ms, SoundGenerator* g) : t(ms/1000.0), sound(g){};
			
		float t;
		SoundGenerator* sound;
	};

	public:
		ChainSound() : SoundGenerator("chain") {}

		ChainSound(istream& in);

		void add(uint32_t ms, SoundGenerator* g);

		virtual void reset();

		virtual void next(float &left, float &right, float speed=1.0);

		virtual void help(Help& help) const;

	private:
		virtual SoundGenerator* build(istream& in) const
		{ return new ChainSound(in); }

		list<ChainElement> sounds;
		list<ChainElement>::iterator it;

		AdsrGenerator* adsr;
		uint16_t gaps;
		float dt;
		float t;
};

#endif
