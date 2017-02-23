#include <libsynth.hpp>
#include <math.h>

Filter::Filter()
: lleft(0), lright(0)
{
}


Filter::Filter(istream& in)
: lleft(0), lright(0)
{
    string s;
	freq = readFrequency(in);
    
	coeff = expf(-2*M_PI*freq/SoundGenerator::samplesPerSeconds());
	if (coeff>1) coeff=1;
	if (coeff<0) coeff=0;
	mcoeff = 1 - coeff;
	
    generator = SoundGenerator::factory(in);
}
