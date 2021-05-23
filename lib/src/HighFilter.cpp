#include <libsynth.hpp>

HighFilter::HighFilter(istream& in)
: Filter(in)
{
	coeff = 1.0 - coeff;
	mcoeff = 1.0 - mcoeff;
}

void HighFilter::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("high", "High pass filter");
    entry->addOption(new HelpOption("freq", "Frequency cutoff", HelpOption::FREQUENCY));
    help.add(entry);
}

void HighFilter::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
	sgfloat  l=0,r=0;
	
	generator->next(l, r, speed);
	lleft = lleft * mcoeff + l*coeff;
	lright= lright* mcoeff + r*coeff;
	
	left += l - lleft;
	right += r - lright;
	return;
}
