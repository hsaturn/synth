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

uint32_t samples_per_seconds = 48000;

map<string, const SoundGenerator*> SoundGenerator::generators;
string SoundGenerator::last_type;
map<string, string> SoundGenerator::defines;
bool SoundGenerator::echo = true;
uint8_t SoundGenerator::verbose = 0;
bool SoundGenerator::init_done = false;
SDL_AudioDeviceID SoundGenerator::dev;
list<SoundGenerator*> SoundGenerator::list_generator;
uint16_t SoundGenerator::list_generator_size = 0; // avoid mx use
uint16_t SoundGenerator::buf_size;
bool SoundGenerator::saturate = false;
mutex SoundGenerator::mtx;
uint32_t SoundGenerator::wanted_buffer_size = 1024;
uint32_t SoundGenerator::samples_per_seconds = 48000;

// Auto register for the factory
static SquareGenerator gen_sq;
static SinusGenerator gen_si;
static AmGenerator gen_am;
static DistortionGenerator gen_dist;
static FmModulator gen_fm;
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
static LowFilter gen_low;
static HighFilter gen_high;
static TriangleGenerator gen_tri;
static ClampSound gen_clamp;
static BlepOscillator gen_blep;
static ResoFilter gen_reso;

SquareGenerator::SquareGenerator(istream& in)
{
    readFrequencyVolume(in);
    a = 0;
    da = 1.0f / (sgfloat ) SoundGenerator::samplesPerSeconds() ;
    val = 1;
}

bool SquareGenerator::_setValue(string name, istream& in)
{
    if (name != "v" && name != "f")
        return false;

    if (name == "f")
        invert = (sgfloat ) (SoundGenerator::samplesPerSeconds()  >> 1) / freq;

    return true;
}

void SquareGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    a += speed;
    left += (sgfloat ) val * volume;
    right += (sgfloat ) val * volume;
    if (a > invert)
    {
        a -= invert;
        val = -val;
    }
}

void SquareGenerator::help(Help& help) const
{
    help.add(SoundGenerator::addHelpOption(new HelpEntry("square", "square sound")));
}

SinusGenerator::SinusGenerator(istream& in)
{
    readFrequencyVolume(in);
}

bool SinusGenerator::_setValue(string name, istream& in)
{
    if (name == "f")
    {
        in >> freq;
        da = (2 * M_PI * freq) / (sgfloat ) SoundGenerator::samplesPerSeconds() ;
        return true;
    }
    return false;
}

void SinusGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    a += da * speed;
    sgfloat  s = sin(a);
    left += volume * s;
    right += volume * s;
    if (a > 2 * M_PI)
    {
        a -= 2 * M_PI;
    }
}

void SinusGenerator::help(Help& help) const
{
    help.add(addHelpOption(new HelpEntry("sinus", "sinus wave")));
}

DistortionGenerator::DistortionGenerator(istream& in)
{
    level = 1.0f + readFloat(in, 0, 100, "level")/100.0f;
    generator = factory(in, true);
}

void DistortionGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    sgfloat  l=0, r=0;
    generator->next(l, r, speed);
    l *= level;
    r *= level;

    if (l > 1) l = 1;
    else if (l<-1) l = -1;
    if (r > 1) r = 1;
    else if (l<-1) l = -1;

    left += l;
    right += r;
}

void DistortionGenerator::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("distortion", "Distort sound");
    entry->addOption(new HelpOption("lvl", "0..100, level of distorsion"));
    entry->addOption(new HelpOption("sound", "Sound generator to distort", HelpOption::GENERATOR));
    entry->addExample("distorsion 50 sinus 200");
    help.add(entry);
}

LevelSound::LevelSound(istream& in)
{
    level = (readFloat(in, 0, 100, "level")-50) / 50.0f;
}

void LevelSound::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    left += level;
    right += level;
}

FmModulator::FmModulator(istream& in)
{
    in >> min;
    in >> max;
    mod_gen = true;
    mod_mod = false;

    sound = factory(in);

    if (sound == 0)
    {
        if (last_type == "generator")
        {
            mod_gen = true;
            mod_mod = false;
        }
        else if (last_type == "modulator")
        {
            mod_gen = false;
            mod_mod = true;
        }
        else if (last_type == "both")
        {
            mod_gen = true;
            mod_mod = true;
        }
        else
            missingGeneratorExit("417");
    }

    if (sound == 0) sound = factory(in, true);
    modulator = factory(in, true);

    if (min < 0 || min > 2000 || max < 0 || max > 2000 || max < min)
    {
        // this->help(cerr); @TODO
        exit(1);
    }

    max /= 100.0;
    min /= 100.0;
}

void FmModulator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    if (min == max)
    {
        sound->next(left, right, mod_gen ? speed : 1.0f);
        return;
    }

    sgfloat  l = 0, r = 0;
    modulator->next(l, r, mod_mod ? speed : 1.0f); // nbre entre -1 et 1

    l = (l + r) / 2.0;
    l = min + (max - min)*(l + 1.0) / 2.0;

    if (mod_gen) l *= speed;
    sound->next(left, right, l);
}

void FmModulator::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("fm", "Frequency modulation");
    entry->addOption(new HelpOption("min", "0..200% modulation when modulator=-1"));
    entry->addOption(new HelpOption("max", "0..200% modulation when modulator=1"));
    entry->addOption(new HelpOption("spd_mod", "[generator|modulator|both] : Where to apply speed modification ", HelpOption::OPTIONAL | HelpOption::CHOICE));
    entry->addOption(new HelpOption("sound", "What sound to modulate", HelpOption::GENERATOR));
    entry->addOption(new HelpOption("modulator", "Modulation shape (any generator)", HelpOption::GENERATOR));
    entry->addExample("fm 80 120 sq 220 sin 10 : 220Hz square modulated with sinus");
    help.add(entry);
}

MixerGenerator::MixerGenerator(istream& in)
{
    while (in.good())
    {
        SoundGenerator* p = factory(in);
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
}

void MixerGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    if (generators.size() == 0)
        return;
    else if (generators.size() == 1)
    {
        generators.front()->next(left, right, speed);
        return;
    }

    sgfloat  l = 0;
    sgfloat  r = 0;

    for (auto generator : generators)
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

void LeftSound::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    sgfloat  v = 0;
    generator->next(left, v);
}

void LeftSound::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("left", "Keep left part of signal");
    entry->addOption(new HelpOption("sound", "Sound generator to apply on", HelpOption::GENERATOR));
    help.add(entry);
}

RightSound::RightSound(istream& in)
{
    generator = factory(in, true);
}

void RightSound::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    sgfloat  v = 0;
    generator->next(v, right);
}

void RightSound::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("right", "Keep right part of signal");
    entry->addOption(new HelpOption("sound", "Generator to apply on", HelpOption::GENERATOR));
    help.add(entry);
}

EnvelopeSound::EnvelopeSound(istream& in)
{
    int ms;
    index = 0;

    in >> ms;

    if (ms <= 0)
    {
        cerr << "Null duration" << endl;
        exit(1);
    }

    istream* input = 0;
    ifstream file;

    string type;
    in >> type;

    loop = true;
    if (type == "once")
    {
        loop = false;
        in >> type;
    }
    else if (type == "loop")
    {
        in >> type;
    }


    if (type == "data")
        input = &in;
    else if (type == "file")
    {
        string name;
        in >> name;
        file.open(name);
        input = &file;
    }
    else
        cerr << "Unkown type: " << type << endl;
    if (input == 0)
    {
        // this->help(cerr); @TODO
        exit(1);
    }
    sgfloat  v;
    while (input->good())
    {
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

    dindex = 1000.0 * (data.size() - 1) / (sgfloat ) ms / SoundGenerator::samplesPerSeconds() ;
}

void EnvelopeSound::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    if (index > data.size())
        return;

    sgfloat  l = 0;
    sgfloat  r = 0;
    generator->next(l, r, speed);

    index += dindex;

    int idx = (int) index;
    sgfloat  dec = index - idx;

    if (idx < 0) idx = 0;
    if (index >= (sgfloat ) data.size() - 1)
    {
        idx = data.size() - 1;
        if (loop)
            index -= (sgfloat ) data.size() - 1;
    }
    sgfloat  cur = data[idx];
    if (idx + 1 < (int) data.size())
        idx++;
    sgfloat  next = data[idx];

    sgfloat  f = cur + (next - cur) * dec;

    left += l*f;
    right += r*f;
}

void EnvelopeSound::help(Help& help) const {
    // @TODO
    /*HelpEntry* entry = new HelpEntry("envelope", "Linear enveloppe generator (time arguments)");
    entry->addOption(new HelpOption("ms:level","Time and level in millisecond and %"));
    entry->addExample("envelope 1000:100 1500:50");
    out << "envelope {timems} [once|loop] {data} generator" << endl;
    out << "  data is either : file filename, or data v1 ... v2 end" << endl;
    out << "  values are from -200 to 200 (gfloat , >100 may distort sound)" << endl;
     * */ }

MonoGenerator::MonoGenerator(istream& in)
{
    generator = factory(in, true);
}

void MonoGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    sgfloat  l = 0;
    sgfloat  v = 0;
    generator->next(l, v, speed);
    l = (l + v) / 2;
    left += l;
    right += l;
}

void MonoGenerator::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("mono", "Mix left & right channel to monophonic output");
    entry->addOption(new HelpOption("sound", "Sound generator to transform", HelpOption::GENERATOR));
    help.add(entry);
}

AmGenerator::AmGenerator(istream& in)
{
    min = readFloat(in, 0, 300, "min") / 100.0;
    max = readFloat(in, 0, 300, "max") / 100.0;

    generator = factory(in, true);
    if (in.good()) modulator = factory(in, true);
}

void AmGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    sgfloat  l = 0, r = 0;
    generator->next(l, r, speed);

    sgfloat  lv = 0, rv = 0;
    modulator->next(lv, rv, speed);

    lv = min + (max - min)*(lv + 1) / 2;
    rv = min + (max - min)*(rv + 1) / 2;

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

ReverbGenerator::ReverbGenerator(istream& in)
{
    echo = (SoundGenerator::last_type == "echo");

    string s;

    in >> s;
    if (s.find(':') == string::npos)
    {
        cerr << "Missing :  in " << SoundGenerator::last_type << " generator." << endl;
        exit(1);
    }
    sgfloat  ms = atof(s.c_str()) / 1000.0;
    s.erase(0, s.find(':') + 1);
    vol = atof(s.c_str()) / 100.0;

    if (ms <= 0)
    {
        cerr << "Negatif time not allowed." << endl;
        exit(1);
    }

    buf_size = SoundGenerator::samplesPerSeconds()  * ms;

    if (buf_size == 0 || buf_size > 1000000)
    {
        cerr << "Bad Buffer size : " << buf_size << endl;
        exit(1);
    }

    buf_left = new sgfloat [buf_size];
    buf_right = new sgfloat [buf_size];
    for (uint32_t i = 0; i < buf_size; i++)
    {
        buf_left[i] = 0;
        buf_right[i] = 0;
    }
    index = 0;
    ech_vol = 1.0 - vol;
    ech_vol = 1.0;

    generator = factory(in, true);
}

void ReverbGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    sgfloat  l = 0;
    sgfloat  r = 0;
    generator->next(l, r, speed);

    if (echo)
    {
        sgfloat  ll = buf_left[index];
        sgfloat  rr = buf_right[index];

        buf_left[index] = l;
        buf_right[index] = r;

        l = l * ech_vol + ll*vol;
        r = r * ech_vol + rr*vol;
    }
    else
    {
        l = l * ech_vol + buf_left[index] * vol;
        r = r * ech_vol + buf_right[index] * vol;

        buf_left[index] = l;
        buf_right[index] = r;
    }
    index++;
    if (index == buf_size)
        index = 0;

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

AdsrGenerator::AdsrGenerator(istream& in)
{
    value v;
    value prev;
    prev.s = 0;
    prev.vol = 0;

    while (read(in, v))
    {
        if (v <= prev)
        {
            cerr << "ADSR times must be ordered." << endl;
            cerr << "previous ms=" << prev.s << " current=" << v.s << endl;
            exit(1);
        }
        values.push_back(v);
        prev = v;
    }

    if (values.size() == 0)
    {
        cerr << "ADSR must contains at least 1 value at t>0ms." << endl;
        exit(1);
    }

    generator = factory(in, true);
    dt = 1.0 / (sgfloat ) SoundGenerator::samplesPerSeconds() ;
    reset();
}

void AdsrGenerator::reset()
{
    t = 0;
    index = 0;
    target = values[0];
    previous.s = 0;
    previous.vol = 0;
}

bool AdsrGenerator::read(istream& in, value& val)
{
    string s;
    in >> s;
    if (s == "once")
    {
        loop = false;
        return false;
    }
    else if (s == "loop")
    {
        loop = true;
        return false;
    }

    if (s.find(':') == string::npos)
    {
        cerr << "Missing : in value or type (once/loop) for adsr." << endl;
        exit(1);
    }
    val.s = atof(s.c_str()) / 1000;
    s.erase(0, s.find(':') + 1);
    val.vol = atof(s.c_str()) / 100;
    return true;
}

void AdsrGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    if (generator == 0)
        return;

    sgfloat  l = 0;
    sgfloat  r = 0;
    generator->next(l, r, speed);

    if (index >= values.size())
    {
        left += l * target.vol;
        right += r * target.vol;
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

    sgfloat  factor = (t - previous.s) / (target.s - previous.s);
    sgfloat  vol = previous.vol + (target.vol - previous.vol) * factor;

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

AvcRegulator::AvcRegulator(istream& in)
{
    stringstream::pos_type last = in.tellg();
    float f;
    in >> f;

    // TODO samplesPerSeconds dependant
    if (f == 0)
    {
      in.clear();
      in.seekg(last);
    }
    else
      factor = f;

    min_gain = 0.05;

    generator = factory(in);

    reset();
}

void AvcRegulator::next(sgfloat & left, sgfloat & right, sgfloat  sp)
{
    if (generator)
    {
        sgfloat  l = 0;
        sgfloat  r = 0;
        generator->next(l, r, sp);

        l *= gain;
        r *= gain;

        if (l > 0.95 || l<-0.95 || r > 0.95 || r<-0.95)
        {
            gain *= factor;
            if (gain < min_gain)
                gain = min_gain;
        }
        else if (gain < 1.0)
            gain *= 1.00001;    // TODO samplesPerSeconds dependant

        left += l;
        right += r;
    }
}

void AvcRegulator::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("avc", "Automatic volume control");
    entry->addOption(new HelpOption("#", "Factor (1 none, 0.7 strongest"));
    entry->addOption(new HelpOption("sound", "Affected sound generator", HelpOption::GENERATOR));
    help.add(entry);
}

void ChainSound::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("chain", "Chain sounds in sequence");
    entry->addOption(new HelpOption("adsr", "Sounds will be played with an adsr", HelpOption::GENERATOR));
    entry->addOption(new HelpOption("ms xxx", "Set default duration (can appear many times"));
    entry->addOption(new HelpOption("mix {ms}", "Set mix duration between sounds (default "+to_string(mix_t)+")"));
    entry->addOption(new HelpOption("gaps ms", "Gaps between sounds (can appear many times)", HelpOption::OPTIONAL));
    entry->addOption(new HelpOption("[ms|x#] sound", "Duration (absolute or default multiplied) and sound generator", HelpOption::REPEAT));
    entry->addOption(new HelpOption("gen generator", "Avoid name of generator for the chain (ex: chain gen sinus 100 200 300 400 end)"));
    entry->addOption(new HelpOption("loop", "loop sequence (and end of chain generator"));
    entry->addOption(new HelpOption("end", "end of sequence"));
    help.add(entry);
}

ChainSound::ChainSound(istream& in)
{
    adsr = 0;
    uint32_t ms = 0;
    uint32_t def_ms = 0;
    string generator;
    while (in.good())
    {
        string sms;
        stringstream::pos_type last = in.tellg();

        in >> sms;
        if (sms == "end")
            break;

        if (sms == "loop")
        {
            loop = true;
            break;
        }

        if (sms == "mix")
        {
            in >> mix_t;
            mix_t /= 1000.0;
        }
        else if (sms == "gen")
        {
            in >> generator;
        }
        else if (sms == "ms")
        {
            in >> def_ms;
            if (def_ms <= 0)
            {
                cerr << "Chain: Cannot have null default duration" << endl;
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
                cerr << "Chain: Cannot have multiple adsr" << endl;
                exit(1);
            }
        }
        else
        {
            uint32_t delta;
            if (sms[0]=='x')
            {
                sms.erase(0,1);
                delta = atol(sms.c_str()) * def_ms;
            }
            else
                delta = atol(sms.c_str());

            if (delta == 0)
            {
                in.clear();
                in.seekg(last);
                delta = def_ms;
                if (delta == 0)
                {
                    cerr << "Chain: Missing duration (and/or no default duration)" << endl;
                    exit(1);
                }
            }
            ms += delta;

            SoundGenerator* sound = factory(generator, in);
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
    dt = 1.0 / (sgfloat ) SoundGenerator::samplesPerSeconds() ;
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

void ChainSound::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
    if (it==sounds.end() and loop) reset();
    if (it != sounds.end())
    {
        t += dt;
        ChainElement& sound = *it;
        float mix_factor = 1.0; // old factor vs next_sound
        SoundGenerator* next_sound = nullptr;
        if (t >= sound.t)
        {
            if (mix_t != 0)
            {
                auto next = it; ++next;
                sgfloat dt=0;
                if (next == sounds.end())
                {
                    next = sounds.begin();
                }
                next_sound = next->sound;
                mix_factor = (sound.t + mix_t - t)/mix_t;
            }

            if (t >= sound.t + mix_t or next_sound == nullptr)
            {
                it++;
                if (adsr)
                {
                    adsr->reset();
                    adsr->setSound(it->sound);
                }
            }
        }
        if (sound.sound)
        {
            float next_left, next_right;
            if (next_sound)
            {
                next_left = left;
                next_right = right;
            }
            if (adsr)
                adsr->next(left, right, speed);
            else
                sound.sound->next(left, right, speed);
            if (next_sound)
            {
                if (adsr)
                {
                    adsr->reset();
                    adsr->setSound(next_sound);
                    adsr->next(next_left, next_right, speed);
                }
                else
                    next_sound->next(next_left, next_right, speed);

                left = mix_factor*left + (1.0-mix_factor)*next_left;
                right = mix_factor*right + (1.0-mix_factor)*next_right;
            }
        }
    }
}

#endif /* LIBSYNTH_HPP */

