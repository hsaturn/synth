#include <libsynth.hpp>

TriangleGenerator::TriangleGenerator(istream& in)
{
	readFrequencyVolume(in);
	sign = 1;
	
	setValue("type", in);
	
	da = 4 / ((float) SoundGenerator::samplesPerSeconds()  / freq);
	if (dir != BIDIR)
		da /= 2.0;
	
	mda = -da;
	pda = da;
	
	reset();
}

bool TriangleGenerator::_setValue(string name, istream& in)
{
	dir = BIDIR;
	if (!in.good())
		return false;

	if (name == "type")
	{
		stringstream::pos_type last = in.tellg();
		string asc_desc;
		in >> asc_desc;
		
		if (asc_desc == "asc")
		{
			dir = ASC;
			sign = 1;
		}
		else if (asc_desc == "desc")
		{
			sign = -1;
			dir = DESC;
		}
		else
		{
			in.clear();
			in.seekg(last);
			return false;
		}
		return true;
	}
	else if (name=="f")
	{
		float f;
		in >> f;
		if (f <= 0)
			cerr << "Triangle : Invalid frequency" << endl;
		else
		{
			freq = f;
			da = 4 / ((float) SoundGenerator::samplesPerSeconds()  / freq);
			return true;
		}
	}
	return false;
}

void TriangleGenerator::reset()
{
	if (dir == ASC)
    {
        sign = 1;
		a = -1;
    }
	else if (dir == DESC)
    {
        sign = -1;
		a = 1;
    }
	else
		a = 0;
}

void TriangleGenerator::next(float& left, float& right, float speed)
{
	a += da;
    
    if (a>1.0)
    {
		if (dir == ASC)
			a = a - 2.0;
		else
		{
			a = 2.0-a;
			da=mda;
		}
    }
    else if (a<-1.0)
    {
		if (dir == DESC)
			a = a + 2.0;
		else
		{
			a = -2 -a;
			da=pda;
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

