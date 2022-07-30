#include "ds-trigger-control.h"
#include <SDL.h>
#include <string.h>
#include <assert.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

namespace triggercontrol
{
    namespace ds
    {

        // I spent so long realizing that it was copying the pointer instead of modifying the pointer's address :/
        int find(SDL_GameController **handle)
        {
            if (SDL_NumJoysticks() < 1)
                return -1;

            for (int i = 0; i < SDL_NumJoysticks(); i++)
            {
                if (SDL_IsGameController(i))
                {
                    *handle = SDL_GameControllerOpen(i);
                    if (*handle)
                    {
                        if (SDL_GameControllerGetType(*handle) == SDL_CONTROLLER_TYPE_PS5)
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        SDL_GameControllerClose(*handle);
                    }
                }
            }
            return -1;
        }

        static uint8_t outReport[70] = {0};

        int apply_effect(SDL_GameController *handle, triggercontrol::ds::triggers trigger, triggercontrol::ds::modes mode, uint8_t effects[7])
        {
            SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
            assert(SDL_GameControllerGetType(handle) == SDL_CONTROLLER_TYPE_PS5);
            if (outReport[0] == 0)
            {
                outReport[0] = 0x2;
                outReport[1] = 0x04 | 0x08;
                outReport[2] = 0x40;
            }
            if (trigger == triggercontrol::ds::triggers::right)
            {
                outReport[11] = static_cast<uint8_t>(mode);
                memcpy(outReport + 12, effects, 6);
                outReport[20] = effects[6];
            }
            else
            {
                outReport[22] = static_cast<uint8_t>(mode);
                memcpy(outReport + 23, effects, 6);
                outReport[30] = effects[6];
            }
            return SDL_GameControllerSendEffect(handle, outReport + 1, ARRAY_SIZE(outReport));
        }

        int reset(SDL_GameController *handle, triggercontrol::ds::triggers trigger)
        {
            assert(SDL_GameControllerGetType(handle) == SDL_CONTROLLER_TYPE_PS5);
            // work around a gcc "bug"
            uint8_t arr[7] = {0};
            return triggercontrol::ds::apply_effect(handle, trigger, triggercontrol::ds::modes::Rigid_B, arr);
        }

        int reset_all(SDL_GameController *handle)
        {
            assert(SDL_GameControllerGetType(handle) == SDL_CONTROLLER_TYPE_PS5);
            // work around a gcc "bug"
            uint8_t arr[7] = {0};
            int err = triggercontrol::ds::apply_effect(handle, triggercontrol::ds::triggers::left, triggercontrol::ds::modes::Rigid_B, arr);
            if (err != 0)
                return err;
            return triggercontrol::ds::apply_effect(handle, triggercontrol::ds::triggers::right, triggercontrol::ds::modes::Rigid_B, arr);
        }

    };
};
