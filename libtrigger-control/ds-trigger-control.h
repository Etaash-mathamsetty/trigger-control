#include <SDL_gamecontroller.h>
#include <inttypes.h>

// you can include this file (along with the cpp file) in your SDL2 project to control the dualsense triggers
#ifndef _DS_TRIGGER_CONTROL_H_
#define _DS_TRIGGER_CONTROL_H_

//in case someone stole the ds namespace
namespace triggercontrol{
namespace ds{

enum class modes
{
	Off = 0x0,	 // no resistance
	Rigid = 0x1, // continous resistance
	Pulse = 0x2, // section resistance
	Rigid_A = 0x1 | 0x20, 
	Rigid_B = 0x1 | 0x04,
	Rigid_AB = 0x1 | 0x20 | 0x04,
	Pulse_A = 0x2 | 0x20,
	Pulse_B = 0x2 | 0x04,
	Pulse_AB = 0x2 | 0x20 | 0x04,
	INVALID = 0xFFFF
};

enum class triggers
{
    left,
    right,
};

//returns -1 for no PS5 controller found, 0 for success
//this function is optional, you just need to have a SDL_GameController* ptr for a ps5 controller
//to use this function pass in a ptr to a SDL_GameController* 
int find(SDL_GameController **handle);

//these functions return -1 for fail, 0 for success


//pass in a SDL_GameController* ptr to a ps5 controller
//pass in which trigger (left or right)
//pass in the trigger mode
//pass in an array of 7 uint8_ts to change the different parameters for each mode
int apply_effect(SDL_GameController* handle, triggercontrol::ds::triggers trigger, triggercontrol::ds::modes mode, uint8_t effects[7]);

//pass in a SDL_GameController* ptr to a ps5 controller
//pass in which trigger you want to reset (left or right)
//you might wan tot add a delay (my project uses 70ms) between this and apply_effect, otherwise the motors will not fully retract
int reset(SDL_GameController* handle, triggercontrol::ds::triggers trigger);

//pass in a SDL_GameController* ptr to a ps5 controller
//it resets both triggers (basically a helper function for reset)
//you might wan tot add a delay (my project uses 70ms) between this and apply_effect, otherwise the motors will not fully retract
int reset_all(SDL_GameController* handle);


}
}

#endif
