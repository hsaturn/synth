#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <cmath>

using namespace std;

using Notes=set<string>;

// used to generate frequencies.def

string default_base="octave 0\n"
"261.63 DO SI# C\n"
"277.18 DO# REb C# Db\n"
"293.66 RE D\n"
"311.13 RE# MIb D# Eb\n"
"329.63 MI FAb E Fb\n"
"349.23 FA MI# F E#\n"
"369.99 FA# SOLb F# Gb\n"
"392.00 SOL G\n"
"415.30 SOL# LAb G# Ab\n"
"440.00 LA A\n"
"466.16 LA# SIb A# Bb\n"
"493.88 SI DOb B Cb";

int main(int argc, const char* argv[])
{
	map<float, Notes> base;

  istream* basedef = nullptr;
  int row=0;
  int base_octave=0;
  int from_octave=-1;
  int to_octave=9;

  int i=1;
  while(i < argc)
	{
		string arg = argv[i];
		if (arg=="-h" or arg=="--help")
		{
			cout << endl << "usage: freq_gen [options] (> frequencies.def)" << endl
			<< endl
			<< "  options: " << endl
			<< "     -from octave   : start from octave" << endl
			<< "     -to octave     : end at octave" << endl
			<< "     -b base_file   : change default frequencies base" << endl
			<< "     -t #           : transpose distance" << endl
			<< endl
			<< "     generate a base.def file: freq_gen -from 0 -to 0 > base.def" << endl
			<< endl;
			return -1;
		}
		else if (arg=="-from")
		{
			from_octave = atol(argv[++i]);
		}
		else if (arg=="-t")
		{
			base_octave = atol(argv[++i]);
		}
		else if (arg=="-b")
		{
			basedef = new ifstream(argv[++i]);
			if (not basedef->good())
			{
				cerr << "Unable to open base file (" << argv[++i] << ")" << endl;
				return -1;
			}
		}
		else if (arg=="-to")
		{
			to_octave = atol(argv[++i]);
		}
		else
		{
			cerr << "Unknown option " << arg << endl;
			return -1;
		}
		++i;
	}
	cerr << "octave range [" << from_octave << ", " << to_octave << ']' << endl;

	if (basedef == nullptr)
	{
  	// b("base.def");
  	auto *stream = new stringstream;
  	stream->str(default_base);
  	basedef=stream;
	}
  while (basedef->good())
	{
		++row;
		string line;
		getline(*basedef, line);
		stringstream parse;
		parse.str(line);
    stringstream::pos_type pos = parse.tellg();

		float freq=0;
		parse >> freq;

		if (freq)
		{
			Notes notes;

			while(parse.good())
			{
				string note;
				parse >> note;
				notes.insert(note);
			}
			base[freq] = notes;
		}
		else
		{
			parse.clear();
			parse.seekg(pos);
			string what;
			parse >> what;
			if (what=="octave")
			{
				int octave;
				parse >> octave;
				base_octave = octave - base_octave;
			}
			else if (what.length() && what[0]!='#')
			{
				cerr << "Syntax error in def file#" << row << ": " << what << '.' << endl;
			}

		}
	}

/*
	cout << "# OCTAVE : " << base_octave << endl;
	for(const auto& elt: base)
	{
		cout << "F=" << elt.first << "\t";
		for(const auto& note: elt.second)
		{
			cout << ' ' << note;
		}
		cout << endl;
	}
	*/

	for(int octave=from_octave; octave <= to_octave; octave++)
	{
		cout << "# octave " << octave << endl;

		int power = octave - base_octave;

		for(const auto& elt: base)
		{
			float freq = elt.first;
			cout << freq * pow(2, power);
			for(const auto& note: elt.second)
			{
				cout << ' ' << note;
				if (octave) cout << octave;
			}
			cout << endl;
		}
		cout << endl;
	}
	return 0;
}
