#include <libsynth.hpp>

void LowFilter::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("low", "Low frequency filter");
    entry->addOption(new HelpOption("freq", "Frequency cutoff", HelpOption::FREQUENCY));
    help.add(entry);
}

void LowFilter::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
	sgfloat  l=0,r=0;
	
	generator->next(l, r, speed);
	l = lleft * coeff + mcoeff * l;
	r = lright* coeff + mcoeff * r;
	left += l;
	right += r;
	lleft = l;
	lright = r;
	return;
}
