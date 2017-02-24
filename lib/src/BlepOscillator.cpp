#include <libsynth.hpp>
inline sgfloat poly_blep(sgfloat t, sgfloat dt)
{
	if (t < dt)
	{
		t = t/dt - 1;
		return -t*t;
	}
	else if (t > 1 - dt)
	{
		t = (t - 1)/dt + 1;
		return t*t;
	}
	return 0;
}
void BlepOscillator::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("blep", "Blep oscillator");
	entry->addOption(new HelpOption("freq", "Frequency", HelpOption::FREQUENCY));
	entry->addOption(new HelpOption("ratio", "Periodic ratio", HelpOption::FLOAT_ONE));
	help.add(entry);
}

BlepOscillator::BlepOscillator(istream& in)
: phase(0)
{
	freq = readFrequency(in);
	pw = readFloat(in, 0, 1, "ratio");
	update();
}

void BlepOscillator::update()
{
	phase_inc = freq / (sgfloat) samplesPerSeconds();
}

void BlepOscillator::next(sgfloat& left, sgfloat& right, sgfloat speed)
{
	sgfloat sample;
	
	if (phase < pw)
		sample = 1.0f;
	else
		sample = -1.0f;
	
	sample += poly_blep(phase, phase_inc);
	
	sgfloat phase2 = phase + 1.0f - pw;
	phase2 = phase2 - floor(phase2);
	sample -= poly_blep(phase2, phase_inc);
	phase += phase_inc;
	phase -= floor(phase);
	
	left += sample;
	right += sample;
}
