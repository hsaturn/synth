#include <libsynth.hpp>

HighFilter::HighFilter(istream& in)
: Filter(in)
{
	coeff = 1.0 - coeff;
	mcoeff = 1.0 - mcoeff;
}

void HighFilter::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("high", "Low frequency filter");
    entry->addOption(new HelpOption("freq", "Frequency cutoff", HelpOption::FREQUENCY));
    help.add(entry);
}

void HighFilter::next(float& left, float& right, float speed)
{
	float l=0,r=0;
	
	
	
	generator->next(l, r, speed);
	l = lleft * coeff + mcoeff * l;
	r = lright* coeff + mcoeff * r;
	lleft = l;
	lright = r;
	
	left += l;
	right += r;
	return;
}