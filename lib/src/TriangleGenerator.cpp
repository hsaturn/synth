#include <libsynth.hpp>

TriangleGenerator::TriangleGenerator(istream& in)
{
	dir = BIDIR;
	ton = 0.5;
	readFrequencyVolume(in);
	
	setValue("type", in);
	
	if (eatWord(in, "ton"))
		setValue("ton", in);
	
	setValue("f", freq);
	
	reset();
}

bool TriangleGenerator::_setValue(string name, istream& in)
{
	if (!in.good())
		return false;

	if (name == "type")
	{
		stringstream::pos_type last = in.tellg();
		string asc_desc;
		in >> asc_desc;
		
		if (asc_desc == "asc")
			dir = ASC;
		else if (asc_desc == "desc")
			dir = DESC;
		else
		{
			in.clear();
			in.seekg(last);
			return false;
		}
		setValue("freq", freq);
		return true;
	}
	else if (name=="ton")
		ton = readFloat(in, 0, 100, "ton") / 100.0;
	else if (name=="f")
	{
		sgfloat  f = readFrequency(in);
		freq = f;

		sgfloat  nech =(sgfloat ) SoundGenerator::samplesPerSeconds() / freq;

		if (dir == BIDIR)
		{
			asc_da = 2.0 / ((sgfloat )nech*ton);
			desc_da =  2.0 / ((sgfloat )nech*(ton - 1));
		}
		else
		{
			asc_da = 1 / (sgfloat )nech;
			desc_da = - asc_da;
		}
		if (dir == DESC)
			da = desc_da;
		else if (dir == ASC)
			da = asc_da;
		else
		{
			if (da>0)
				da = asc_da;
			else
				da = desc_da;
		}
		return true;
	}
	return false;
}

void TriangleGenerator::reset()
{
	if (dir == ASC)
	{
		a = -1;
		da = asc_da;
	}
	else if (dir == DESC)
	{
		a = 1;
		da = desc_da;
	}
	else
		a = 0;
}

void TriangleGenerator::next(sgfloat & left, sgfloat & right, sgfloat  speed)
{
	a += da;

    if (a>1.0)
    {
		if (dir == ASC)
		{
			a = a - 2.0;
		}
		else
		{
			a = 2.0-a;
			da=desc_da;
		}
    }
    else if (a<-1.0)
    {
		if (dir == DESC)
			a = a + 2.0;
		else
		{
			a = -2.0 -a;
			da=asc_da;
		}
    }
	left += a * volume;
	right += a * volume;
}

void TriangleGenerator::help(Help& help) const
{
	HelpEntry* entry = SoundGenerator::addHelpOption(new HelpEntry("triangle","triangle sound"));
	entry->addOption(new HelpOption("type", "[tri|asc|desc] Type of signal triangle or asc/desc sawtooth (default  :tri)", HelpOption::OPTIONAL | HelpOption::CHOICE));
	help.add(entry);
}

