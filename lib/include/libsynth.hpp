#ifndef LIBSYNTHETISER
#    define LIBSYNTHETISER

#    include <SDL2/SDL.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <math.h>
#    include <iostream>
#    include <stdint.h>
#    include <string.h>
#    include <list>
#    include <sstream>
#    include <fstream>
#    include <map>
#    include <vector>
#    include <chrono>
#    include <atomic>
#    include <mutex>
#    include <memory>

using namespace std;

typedef float sgfloat;

class SoundGenerator
{
  public:
	static bool init();

	static void quit();

	virtual ~SoundGenerator() { };

	string name;

	// Next sample to add to left and right 
	// added value should be from -1 to 1
	// speed = samples/sec modifier
	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) = 0;

	virtual void reset() { };

	bool setValue(string name, sgfloat  value);
	bool setValue(string name, string value);
	bool setValue(string name, istream& value);

	virtual string getValue(string name) const
	{
		return "?";
	};

	virtual bool isValid() const
	{
		return true;
	}

	static SoundGenerator* factory(istream& in, bool needed = false);
	static SoundGenerator* factory(string type, istream& in);

	static SoundGenerator* factory(string s);
	static string getTypes();

	/**
	 * @return -1..1 gfloat 
	 */
	static sgfloat  rand();

	static void help();

	static string last_type;

	static void missingGeneratorExit(string msg = "");

	static void play(SoundGenerator*); // Add it if necessary
	static bool stop(SoundGenerator*);
	static bool remove(SoundGenerator*); // Remove it
	static bool has(SoundGenerator*, bool bLock = false); // Does it playing ?
	
	/**
	 * Eat expected word if exist else 'in' is left unchanged and false is returned
	 * @param in
	 * @param expected
	 * @return 
	 */
	static bool eatWord(istream &in, string expected);
	
	/**
	 * Get a gfloat  if available and issue either a warning or an error if out of range / not present
	 * @param in
	 * @param min
	 * @param max
	 * @param varname
	 * @return 
	 */
	static sgfloat  readFloat(istream &in, sgfloat  min, sgfloat  max, string varname);
	
	static sgfloat	readFrequency(istream &, string name="");
	
	/**
	 * remove space tab, cr and lf from in and return first non blank character or 0 if bad 
	 * @param in
	 * @return 
	 */
	static char trim(istream& in);

	// Return the number of active playing generators.

	static uint16_t count()
	{
		return list_generator_size;
	}

	static uint32_t samplesPerSeconds()
	{
		return samples_per_seconds;
	}

	static uint16_t bufSize()
	{
		return buf_size;
	}

	class HelpOption
	{
		typedef uint16_t flag_type;
	  public:
		static const flag_type OPTIONAL = 1; // Option is optional
		static const flag_type REPEAT = 2; // Option can be repeated
		static const flag_type INPUT = 4;
		static const flag_type GENERATOR = 8; // Name of the option IS the registered name of a SoundGenerator
		static const flag_type MS_VOL = 16; // Time in ms (gfloat ) / Volume in % couple.
		static const flag_type FREQ_VOL = 32; // Frequency in HZ (gfloat ) / Volume in % couple.
		static const flag_type CHOICE = 64; // Dropdown List of fixed values (*MUST* appear at beginning of arg_desc with the format [opt1|opt2...]
		static const flag_type FREQUENCY = 128;
		static const flag_type FLOAT_ONE = 256;	// Float from -1 to 1

		HelpOption(string arg_name, string arg_desc, flag_type opt_flags = 0) : name(arg_name), desc(arg_desc), flags(opt_flags) { };

		const string& getName() const
		{
			return name;
		}

		const string& getDesc() const
		{
			return desc;
		}
		string str() const;

		bool isOptional() const
		{
			return flags & OPTIONAL;
		}

		bool isRepeatable() const
		{
			return flags & REPEAT;
		}

	  private:
		string name;
		string desc;
		flag_type flags;
	};

	class HelpEntry
	{
	  public:

		/**
		 * @param command : $ is replaced by options, else, they are appended
		 * @param description
		 */
		HelpEntry(string command, string description) : cmd(command), desc(description) { }

		void addOption(HelpOption* option)
		{
			options.push_back(shared_ptr<HelpOption>(option));
		}

		void addExample(string ex)
		{
			example = ex;
		}
		string concatOptions() const;
		string getFullCmd() const;

		const string& getCmd() const
		{
			return cmd;
		}

		const string& getDesc() const
		{
			return desc;
		}

		const string& getExample() const
		{
			return example;
		}

		const list<shared_ptr<HelpOption>>&getOptions() const
		{
			return options;
		}

	  private:
		string cmd;
		string desc;
		string example;
		list<shared_ptr<HelpOption>> options;
	};

	class Help
	{
	  public:

		void add(HelpEntry* entry)
		{
			entries.push_back(shared_ptr<HelpEntry>(entry));
		}
		friend class SoundGenerator;

		friend ostream& operator <<(ostream& out, const Help&);

		Help() { };

		static string padString(string s, string::size_type length);

	  private:

		string cmd;
		string desc;
		list<shared_ptr<HelpEntry>> entries;
	};
  protected:
	virtual bool _setValue(string name, istream& value);

	SoundGenerator() { };

	// Auto register for the factory
	SoundGenerator(string name);

	bool readFrequencyVolume(istream &in);

	virtual SoundGenerator* build(istream& in) const = 0;
	virtual void help(Help& help) const;
	void help(ostream&) const;
	HelpEntry* addHelpOption(HelpEntry*) const;

	// Main audio callback
	static void audioCallback(void *unused, Uint8 *byteStream, int byteStreamLength);

	sgfloat  volume;
	sgfloat  freq;

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
	static uint8_t verbose;
	static bool init_done;
	static SDL_AudioDeviceID dev;
	static list<SoundGenerator*> list_generator;
	static uint16_t list_generator_size;
	static mutex mtx;
	static uint16_t buf_size;
	static bool saturate;
	static uint32_t wanted_buffer_size;
	static uint32_t samples_per_seconds;
	static SDL_AudioSpec have;
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
	SoundGenerator(name) { }

	SoundGeneratorVarHook(istream &in, atomic<T>* v, T min, T max)
	:
	mref(v), mmin(min), mmax(max) { }

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0)
	{
		sgfloat  delta = 2.0 * (sgfloat )(*mref - mmin) / (sgfloat )(mmax - mmin) - 1.0;

		left += delta;
		right += delta;
	}

	virtual void help(ostream &out) const
	{
		out << "Help not defined (SoundGeneratorVarHook)" << endl;
	}

  protected:

	virtual SoundGenerator* build(istream &in) const
	{
		return new SoundGeneratorVarHook<T>(in, mref, mmin, mmax);
	}

  private:
	atomic<T>* mref;
	T mmin;
	T mmax;
};

class WhiteNoiseGenerator : public SoundGenerator
{
  public:

	WhiteNoiseGenerator() : SoundGenerator("wnoise") { } // factory

	WhiteNoiseGenerator(istream& in) { };

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0)
	{
		left += SoundGenerator::rand();
		right += SoundGenerator::rand();
	}

  protected:

	virtual SoundGenerator* build(istream &in) const
	{
		return new WhiteNoiseGenerator(in);
	}

	virtual void help(Help& help) const
	{
		help.add(new HelpEntry("wnoise", "Generator stereo white noise"));
	}

};

class TriangleGenerator : public SoundGenerator
{
	const uint8_t ASC = 0;
	const uint8_t DESC = 1;
	const uint8_t BIDIR = 2;

  public:

	TriangleGenerator() : SoundGenerator("tri triangle") { }; // factory

	TriangleGenerator(istream& in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0);
	virtual void reset();

  protected:
	virtual bool _setValue(string name, istream& in);

	virtual SoundGenerator* build(istream& in) const
	{
		return new TriangleGenerator(in);
	}

	virtual void help(Help& help) const;

  private:
	sgfloat  a;
	sgfloat  da;
	sgfloat  asc_da;		// negative steps
	sgfloat  desc_da;		// negative steps
	sgfloat  ton;
	uint8_t dir;
};

class SquareGenerator : public SoundGenerator
{
  public:

	SquareGenerator() : SoundGenerator("sq square") { };

	SquareGenerator(istream& in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0);


  protected:
	virtual bool _setValue(string name, istream& in);

	virtual SoundGenerator* build(istream& in) const
	{
		return new SquareGenerator(in);
	}

	virtual void help(Help& help) const;

  private:
	sgfloat  a;
	sgfloat  da;
	sgfloat  invert;
	int val;
};

class SinusGenerator : public SoundGenerator
{
  public:

	SinusGenerator() : SoundGenerator("sin sinus") { };

	SinusGenerator(istream& in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0);

  protected:
	virtual bool _setValue(string name, istream& in);

	virtual SoundGenerator* build(istream& in) const
	{
		return new SinusGenerator(in);
	}

	virtual void help(Help& help) const;


  private:
	sgfloat  a;
	sgfloat  da;
};

class DistortionGenerator : public SoundGenerator
{
  public:

	DistortionGenerator() : SoundGenerator("distorsion") { }

	DistortionGenerator(istream& in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed) override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

  protected:

	virtual SoundGenerator* build(istream& in) const override
	{
		return new DistortionGenerator(in);
	}

	virtual void help(Help& help) const override;

  private:
	sgfloat  level;
	SoundGenerator* generator;
};

class LevelSound : public SoundGenerator
{
  public:

	LevelSound() : SoundGenerator("level") { }

	LevelSound(istream &in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 0.1) override;

	virtual void help(ostream &out) const
	{
		out << "level value : constant level" << endl;
	}

  protected:

	virtual SoundGenerator* build(istream &in) const override
	{
		return new LevelSound(in);
	}

  private:
	sgfloat  level;
};

class FmModulator : public SoundGenerator
{
  public:

	FmModulator() : SoundGenerator("fm") { } // for thefactory

	FmModulator(istream& in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual bool isValid() const override
	{
		return sound != 0 && modulator != 0;
	}



  protected:

	virtual SoundGenerator* build(istream& in) const override
	{
		return new FmModulator(in);
	}

	virtual void help(Help& help) const override;

  private:
	sgfloat  min;
	sgfloat  max;
	SoundGenerator* sound;
	SoundGenerator* modulator;
	sgfloat  last_ech_left;
	sgfloat  last_ech_right;
	bool mod_gen;
	bool mod_mod;
};

class MixerGenerator : public SoundGenerator
{
  public:

	MixerGenerator() : SoundGenerator("{") { };

	MixerGenerator(istream& in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;


  protected:

	virtual SoundGenerator* build(istream &in) const override
	{
		return new MixerGenerator(in);
	}

	virtual void help(Help& help) const override;

  private:
	list<SoundGenerator*> generators;

};

class LeftSound : public SoundGenerator
{
  public:

	LeftSound() : SoundGenerator("left") { }

	LeftSound(istream& in)
	{
		generator = factory(in, true);
	}

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

  protected:

	virtual SoundGenerator* build(istream& in) const override
	{
		return new LeftSound(in);
	}

	virtual void help(Help& help) const override;


  private:
	SoundGenerator* generator;
};

class RightSound : public SoundGenerator
{
  public:

	RightSound() : SoundGenerator("right") { }
	RightSound(istream &in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

  protected:

	virtual SoundGenerator* build(istream& in) const override
	{
		return new RightSound(in);
	}

	virtual void help(Help& help) const override;

  private:
	SoundGenerator* generator;
};

class ClampSound : public SoundGenerator
{
  public:

	ClampSound() : SoundGenerator("clamp") { }
	ClampSound(istream &in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

  protected:
	virtual SoundGenerator* build(istream &in) const override
	{
		return new ClampSound(in);
	}
	
	void init();
	
	virtual void help(Help& help) const override;
	
  private:
	sgfloat  level;	// -1 .. 1
	SoundGenerator* generator;
};

class EnvelopeSound : public SoundGenerator
{
  public:

	EnvelopeSound() : SoundGenerator("envelope env") { }

	EnvelopeSound(istream &in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

	virtual void help(Help& help) const override;


  protected:

	virtual SoundGenerator* build(istream& in) const override
	{
		return new EnvelopeSound(in);
	}


  private:
	bool loop;
	sgfloat  index;
	sgfloat  dindex;

	vector<sgfloat > data;
	SoundGenerator* generator;
};

class MonoGenerator : public SoundGenerator
{
  public:

	MonoGenerator() : SoundGenerator("mono") { }

	MonoGenerator(istream &in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

	virtual void help(Help& help) const override;

  protected:

	virtual SoundGenerator* build(istream &in) const override
	{
		return new MonoGenerator(in);
	}

  private:
	SoundGenerator* generator;
};

class AmGenerator : public SoundGenerator
{
  public:

	AmGenerator() : SoundGenerator("am") { }; // for the factory

	AmGenerator(istream &in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;
	virtual void help(Help& help) const override;

	virtual bool isValid() const override
	{
		return generator != 0 && modulator != 0;
	}

  protected:

	virtual SoundGenerator* build(istream& in) const override
	{
		return new AmGenerator(in);
	}


  private:
	sgfloat  min;
	sgfloat  max;
	SoundGenerator* generator;
	SoundGenerator* modulator;
};

class ReverbGenerator : public SoundGenerator
{
  public:

	ReverbGenerator() : SoundGenerator("reverb echo") { }

	ReverbGenerator(istream& in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual void help(Help& help) const override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

  protected:

	virtual SoundGenerator* build(istream &in) const override
	{
		return new ReverbGenerator(in);
	}

  private:
	bool echo;
	sgfloat  vol;
	sgfloat  ech_vol;
	sgfloat * buf_left;
	sgfloat * buf_right;
	uint32_t buf_size;
	uint32_t index;
	SoundGenerator* generator;
};


class BlepOscillator : public SoundGenerator
{
  public:

	BlepOscillator() : SoundGenerator("blep") { }

	BlepOscillator(istream& in);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual void help(Help& help) const override;

  protected:

	virtual SoundGenerator* build(istream &in) const override
	{
		return new BlepOscillator(in);
	}
	
	void update();
	
  private:
	sgfloat	phase;
	sgfloat	pw;
	sgfloat	phase_inc;
};


class AvcRegulator : public SoundGenerator
{
  public:

	AvcRegulator() : SoundGenerator("avc") { };

	AvcRegulator(istream &in);

	virtual void reset()
	{
		gain = 1.0f;
	}

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

	virtual void help(Help& help) const override;

  private:

	virtual SoundGenerator* build(istream& in) const override
	{
		return new AvcRegulator(in);
	}

	SoundGenerator* generator;
	sgfloat  speed;
	sgfloat  gain;
	sgfloat  min_gain;
};

class Filter : public SoundGenerator
{
  public:
	Filter(const string &name) : SoundGenerator(name){}
	Filter(istream& in);
	~Filter() {}
	
	virtual bool isValid() const override
	{
		return generator != 0;
	}
	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override=0;
	
  protected:
	Filter();
	
	SoundGenerator* generator;
	sgfloat  lleft;
	sgfloat  lright;
	sgfloat  coeff;
	sgfloat  mcoeff;	// 1.0 - coeff
};

class LowFilter : public Filter
{
  public:

	LowFilter() : Filter("low") { }
	LowFilter(istream& in) : Filter(in) {}
	
	void next(sgfloat & left, sgfloat & right, sgfloat  speed=1.0);
	
  protected:
	
	virtual SoundGenerator* build(istream& in) const
	{
		return new LowFilter(in);
	}

	virtual void help(Help& help) const;
};

class HighFilter : public Filter
{
  public:

	HighFilter() : Filter("high") { }
	HighFilter(istream& in);
	void next(sgfloat & left, sgfloat & right, sgfloat  speed=1.0) override;

  protected:
	
	virtual SoundGenerator* build(istream& in) const override
	{
		return new HighFilter(in);
	}

	virtual void help(Help& help) const override;
};

class ResoFilter : public SoundGenerator
{
  public:

	ResoFilter() : SoundGenerator("reso") { }
	ResoFilter(istream& in);
	virtual ~ResoFilter() {}
	
	virtual bool isValid() const override
	{
		return generator != 0;
	}
	void next(sgfloat & left, sgfloat & right, sgfloat  speed=1.0) override;

  protected:
	
	virtual SoundGenerator* build(istream& in) const override
	{
		return new ResoFilter(in);
	}

	virtual void help(Help& help) const override;

  protected:
	SoundGenerator* generator;

	sgfloat  lleft;
	sgfloat  lright;
	sgfloat	f;
	sgfloat q;
	sgfloat fb;
	sgfloat lbuf0;
	sgfloat lbuf1;
	sgfloat rbuf0;
	sgfloat rbuf1;
};


class IIRFilter : public SoundGenerator
{
  public:

	IIRFilter();
	IIRFilter(istream& in);
	virtual ~IIRFilter() {}
	
	virtual bool isValid() const override
	{
		return generator != 0;
	}
	void next(sgfloat & left, sgfloat & right, sgfloat  speed=1.0) override;

  protected:
	
	virtual SoundGenerator* build(istream& in) const override
	{
		return new IIRFilter(in);
	}

	virtual void help(Help& help) const override;

  protected:
	SoundGenerator* generator;

	sgfloat w,q,rf,c;	// Filter delay (left))
	sgfloat lfb_lp, lfb_hp;		// Storage for calculated feedback (left)
	
	sgfloat vibrapos_l,vibraspeed_l,vibrapos_r,vibraspeed_r;	// Filter delay (left))
	sgfloat rfb_lp, rfb_hp;		// Storage for calculated feedback (left)
	
	sgfloat p4=1.0e-24;		// Pentium 4 denormal problem elimination
	sgfloat freq, magnitude, resofreq, amp;
};


class AdsrGenerator : public SoundGenerator
{
	struct value
	{
		sgfloat  s;
		sgfloat  vol;

		bool operator <=(const value &v)
		{
			return s <= v.s;
		}

		friend ostream& operator <<(ostream &out, const value &v)
		{
			out << '(' << v.s << "s, " << v.vol << ')';
			return out;
		}
	};

  public:

	AdsrGenerator() : SoundGenerator("adsr") { }

	AdsrGenerator(istream& in);

	virtual void reset() override;

	bool read(istream &in, value &val);

	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual void help(Help &) const override;

	virtual bool isValid() const override
	{
		return generator != 0;
	}

	void setSound(SoundGenerator* sound)
	{
		generator = sound;
	}

  protected:

	virtual SoundGenerator* build(istream &in) const override
	{
		return new AdsrGenerator(in);
	}


  private:
	sgfloat  t;
	sgfloat  dt;

	value previous;
	value target;
	uint32_t index;
	vector<value> values;
	SoundGenerator* generator;
	bool loop;
};

class ChainSound : public SoundGenerator
{

		class ChainElement
		{
			public:

			ChainElement(uint32_t ms, SoundGenerator* g) : t(ms / 1000.0), sound(g) { };

			sgfloat  t;
			SoundGenerator* sound;
		};

  public:

		ChainSound() : SoundGenerator("chain") { }

		ChainSound(istream& in);

		void add(uint32_t ms, SoundGenerator* g);

		virtual void reset() override;

		virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

		virtual bool isValid() const override
		{
			return true;
		}


		virtual void help(Help& help) const override;

  private:

		virtual SoundGenerator* build(istream& in) const override
		{
			return new ChainSound(in);
		}

		list<ChainElement> sounds;
		list<ChainElement>::iterator it;

		AdsrGenerator* adsr;
		uint16_t gaps;
		sgfloat  dt;
		sgfloat  t;
		bool		 loop = false;
};

class Oscilloscope : public SoundGenerator
{

	class Buffer
	{

		struct Max
		{
			sgfloat  value;
			uint32_t pos;
		};

	  public:
		Buffer(uint32_t sz, bool auto_threshold = true);

		void resize(uint32_t sz) { }

		bool fill(sgfloat  left, sgfloat  right);

		void reset();

		~Buffer()
		{
			delete[] buffer;
		}

		void render(SDL_Renderer* r, int w, int h, bool draw_left, sgfloat  dx = 1);

	  private:
		uint32_t size;
		uint32_t pos;
		sgfloat * buffer;
		Max lmax;
		Max rmax;
		bool auto_threshold;
	};

  public:
	Oscilloscope();
	~Oscilloscope();

	Oscilloscope(istream& in);


	virtual void next(sgfloat  &left, sgfloat  &right, sgfloat  speed = 1.0) override;

	virtual bool isValid() const override
	{
		return sound != 0;
	}

  private:

	virtual SoundGenerator* build(istream &in) const override
	{
		return new Oscilloscope(in);
	}

	Buffer* buffer;
	SoundGenerator* sound;
};

#endif
