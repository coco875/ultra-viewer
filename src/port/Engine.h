#pragma once

#include <vector>
#include <Fast3D/gfx_pc.h>
#include "libultraship/src/Context.h"

class GameEngine {
  public:
    static GameEngine* Instance;

    std::shared_ptr<Ship::Context> context;

    GameEngine();
    static void Create();
    void StartFrame() const;
    static void RunCommands(Gfx* Commands);
    static void Destroy();
};

extern float GameEngine_GetAspectRatio();
extern uint8_t GameEngine_OTRSigCheck(char* imgData);
extern float OTRGetDimensionFromLeftEdge(float v);
extern float OTRGetDimensionFromRightEdge(float v);
extern int16_t OTRGetRectDimensionFromLeftEdge(float v);
extern int16_t OTRGetRectDimensionFromRightEdge(float v);