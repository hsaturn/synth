#include "libsynth.hpp"
#include "math.h"

ClampSound::ClampSound(istream& in)
{
	level = fabs(readFloat(in, 0, 100, "level")/100.0);
	generator = factory(in, true);

	init();
}

void ClampSound::init()
{
	
}

void ClampSound::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
	sgfloat  mlevel = -level;
	sgfloat  l = 0;
	sgfloat  r = 0;
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
