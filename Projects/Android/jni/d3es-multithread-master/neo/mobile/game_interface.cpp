
#include "renderer/tr_local.h"
#include "sys/platform.h"

extern "C"
{

static bool inMenu = false;
static bool inGameGuiActive = false;
static bool objectiveSystemActive = false;
static bool inCinematic = false;

extern "C" void Doom3Quest_setUseScreenLayer(bool use);
extern "C" void Doom3Quest_FrameSetup();

void Android_PumpEvents(int screen)
{
	inMenu = screen & 0x1;
	inGameGuiActive = !!(screen & 0x2);
	objectiveSystemActive = !!(screen & 0x4);
	inCinematic = !!(screen & 0x8);

	Doom3Quest_setUseScreenLayer(inMenu || objectiveSystemActive || inCinematic);

	Doom3Quest_FrameSetup();
}

}