#include <libsynth.hpp>

ResoFilter::ResoFilter(istream& in)
{
	f = readFloat(in, 0,1,"f");
	q = readFloat(in, 0,1,"q");
	fb = q + q/(1.0 - f);
	lbuf0 = 0;
	lbuf1 = 0;
	rbuf0 = 0;
	rbuf1 = 0;
	cout << "f " << f << " q " << q <<  endl;
	generator = factory(in);
}

void ResoFilter::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("reso", "Resonant filter");
    entry->addOption(new HelpOption("f", "f parameter [0..1]", HelpOption::FLOAT_ONE));
    entry->addOption(new HelpOption("q", "q parameter [0..1]", HelpOption::FLOAT_ONE));
    help.add(entry);
}

void ResoFilter::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
	sgfloat  l=0,r=0;
	generator->next(l, r, speed);
	
	//set feedback amount given f and q between 0 and 1

	lbuf0 = lbuf0 + f * (l - lbuf0 + fb * (lbuf0 - lbuf1));
	lbuf1 = lbuf1 + f * (lbuf0 - lbuf1);
	left += lbuf1;
	
	rbuf0 = rbuf0 + f * (l - rbuf0 + fb * (rbuf0 - rbuf1));
	rbuf1 = rbuf1 + f * (rbuf0 - rbuf1);
	right += rbuf1;	
	
	return;
}