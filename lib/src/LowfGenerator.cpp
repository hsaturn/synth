#include <libsynth.hpp>

LowFilter::LowFilter(istream& in)
{
    string s;
    in >> freq;
    setFreq(freq);
    
    generator = SoundGenerator::factory(in);
}

void LowFilter::setFreq(float f)
{
    freq = f;
    dmax = 2.0 * freq / (float)SoundGenerator::samplesPerSeconds();
    cout << 2.0 * freq << '/' << SoundGenerator::samplesPerSeconds() << "=" << dmax << endl;
    mdmax = -dmax;
    cout << "dmax=" << dmax << " / " << mdmax << endl;
}

void LowFilter::next(float& left, float& right, float speed)
{
    generator->next(left, right, speed);
    float d=lleft-left;
 
    if (d>dmax)
        left=lleft-dmax;
    else if (d<mdmax)
        left=lleft+dmax;
    
    d = lright - right;
    if (d>dmax)
       right = lright - dmax;
    else if (d<mdmax)
        right = lright + dmax;
    
    lleft = left;
    lright = right;
}

void LowFilter::help(Help& help) const
{
    HelpEntry* entry = new HelpEntry("lowf", "Low frequency filter");
    entry->addOption(new HelpOption("freq", "Frequency cutoff", HelpOption::FREQUENCY));
    help.add(entry);
}
