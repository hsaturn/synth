#include <libsynth.hpp>
#include <math.h>

Filter::Filter()
: lleft(0), lright(0)
{
}


Filter::Filter(istream& in)
{
    string s;
    in >> freq;
    
	coeff = expf(-2*M_PI*freq/SoundGenerator::samplesPerSeconds());
	if (coeff>1) coeff=1;
	if (coeff<0) coeff=0;
	mcoeff = 1 - coeff;
	
	cout << "coeff=" << coeff << " mcoeff=" << mcoeff << endl;
	
    generator = SoundGenerator::factory(in);
}
