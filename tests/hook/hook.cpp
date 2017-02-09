#include "synth.cpp"
#include <atomic>
#include <thread>
#include <unistd.h>

static atomic<float> val(75.1);
static float speed=0;

void speed_evolution()
{
	const int delay=2500;
	while(true)
	{
		for(int i=0; i<400; i++)
		{
			val = val + 0.13;
			if (val>100.0)
				val=100.0;
			
			cout << val << endl;
			usleep(delay*2);
		}
		for(int i=0; i<400; i++)
		{
			val = val - 0.13;
			if (val<0)
				val = 0;
			cout << val << endl;
			usleep(delay);
		}
	}
}

class EngineSpeed : public SoundGeneratorVarHook<float>
{
	public:
		EngineSpeed() : SoundGeneratorVarHook(&val, 0, 100, "engine_speed")
		{
			init();
		}

	protected:
		void init()
		{
			static bool done=false;
			if (done) return;
			done=true;

			thread t(speed_evolution);
			t.detach();
		}
};

static EngineSpeed gen_engine_speed;
