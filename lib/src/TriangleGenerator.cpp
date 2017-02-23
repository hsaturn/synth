#include <libsynth.hpp>

TriangleGenerator::TriangleGenerator(istream& in)
{
	ton = 0.5;
	readFrequencyVolume(in);
	
	setValue("type", in);
	
	if (eatWord(in, "ton"))
	{
		in >> ton;
		ton /= 100.0;
		if (ton>=1 || ton<=0) cerr << "WARNING: Bad TON value (0.100 allowed)" << endl;
		if (ton<0) ton=0;
		if (ton>1) ton=1;
	}
	
	setValue("f", freq);
	
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
			dir = ASC;
		else if (asc_desc == "desc")
			dir = DESC;
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
			
			float nech =(float) SoundGenerator::samplesPerSeconds() / freq;
			
			if (dir == BIDIR)
			{
				asc_da = 2.0 / ((float)nech*ton);
				desc_da =  2.0 / ((float)nech*(ton - 1));
			}
			else
			{
				asc_da = 1 / (float)nech;
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
			cout << "ASC = " << asc_da << " DESC = " << desc_da << " TON=" << ton << endl;
			return true;
		}
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

