#include "libsynth.hpp"
#include "math.h"

ClampSound::ClampSound(istream& in)
{
	in >> level;
	generator = factory(in, true);

	init();
}

void ClampSound::init()
{
	level = fabs(level / 100.0);

	if (level > 1 )
	{
		if (level > 1)
			level = 1;
		cerr << "WARNING: ClampSound value out of range (0..100)" << endl;
	}
	
}

void ClampSound::next(float& left, float& right, float speed)
{
	float mlevel = -level;
	float l = 0;
	float r = 0;
	generator->next(l, r, speed);

	if (l > level)	l = level;
	if (l < mlevel)	l = mlevel;

	if (r > level)	r = level;
	if (r < mlevel)	r = mlevel;
	
	left += l;
	right += r;
}

void ClampSound::help(Help& help) const
{
	HelpEntry* entry = new HelpEntry("clamp", "Limit abruptly signal excursion");
	entry->addOption(new HelpOption("level", "max absolute value",
									HelpOption::INPUT | HelpOption::FLOAT_ONE));
}
