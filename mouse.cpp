// OK bad design, but keep single synth file (yet)
#include "synth.cpp"

// Exemple of external event that can modify the
// generated sound (see mousewheel.synth)
class SoundGeneratorMouseWheelLevel : public SoundGenerator
{
	public:
		SoundGeneratorMouseWheelLevel() : SoundGenerator("mouse_hook")
	{
		init();
	}

		SoundGeneratorMouseWheelLevel(istream &in)
		{
			in >> min;
			in >> max;
		}

		virtual void help(ostream &out)
		{ out << "mouse_hook min max" << endl; }

		virtual void next(float &left, float &right, float speed)
		{
			int x,y;
			SDL_PumpEvents();
			if (SDL_GetMouseState(&x,&y) & SDL_BUTTON(SDL_BUTTON_LEFT))
				exit(1);

			left += min + (max-min)*(float)x/640.0;
			right += min + (max-min)*(float)y/480.0;
		}

	protected:
		virtual SoundGenerator* build(istream &in) const
		{ return new SoundGeneratorMouseWheelLevel(in); }

		void init()
		{
			static bool done=false;
			if (done)
				return;

			if (SDL_Init( SDL_INIT_VIDEO ) == -1)
			{
				cerr << "Cannot init video" << endl;
				exit(1);
			}
			SDL_Window *win = SDL_CreateWindow("Move the mouse here, click to exit", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
			//SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

			if (win == 0)
				exit(1);
		}

	private:
		float min;
		float max;
};

// Now wheel can be used and will return
// a level from 0 to 1 based on mouse wheel movements
static SoundGeneratorMouseWheelLevel gen_mouse;



