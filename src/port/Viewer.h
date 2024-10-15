#pragma once
#include "Fast3D/gfx_pc.h"

class ViewerApp {
public:
    uint16_t buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
    void Setup();
    void Update();
    static void RunFrame();
};