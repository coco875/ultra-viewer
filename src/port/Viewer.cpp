#include "Viewer.h"
#include "libultraship/bridge.h"
#include <libultra/gbi.h>
#include "Engine.h"

#include <libultra/gu.h>

extern "C" {
    void gSPDisplayList(Gfx* pkt, Gfx* dl);
    void gSPVertex(Gfx* pkt, uintptr_t v, int n, int v0);
}

typedef struct {
  float   eye_x;
  float   eye_y;
  float   eye_z;
  float   at_x;
  float   at_y;
  float   at_z;
  float   up_x;
  float   up_y;
  float   up_z;
  float   pitch;
  float   yaw;
  float   roll;
  float   dist;
} CLookAt;

Gfx gGfxPool[1024];
Gfx* gDLMaster = &gGfxPool[0];
Mtx projection;
Mtx translate;
Mtx modeling;
Mtx viewing;
Mtx scale;
u16 framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
u16 zbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

ViewerApp* Instance = new ViewerApp();

Vtx_t gVtx[3] = {
    { { 0, 0, 0 }, 0, 0, 0, 0, 0, 0, 255 },
    { { 50, 0, 0 }, 0, 0, 0, 0, 0, 0, 255 },
    { { 0, 50, 0 }, 0, 0, 0, 0, 0, 0, 255 }
};

static Vp vp = {
    SCREEN_WIDTH*2, SCREEN_HEIGHT*2, G_MAXZ/2, 0,	/* scale */
    SCREEN_WIDTH*2, SCREEN_HEIGHT*2, G_MAXZ/2, 0,	/* translate */
};

std::string test = "__OTR__ast_katina/aDestroyedMothershipDL";

float theta = 0;
float sz = 0.00003f;
uint16_t perspNorm;

void ViewerApp::Setup() {
    // guOrtho(&projection, -(SCREEN_WIDTH / 2.0f), SCREEN_WIDTH / 2.0f, -(SCREEN_HEIGHT / 2.0f), SCREEN_HEIGHT / 2.0f, 1000.0f, -1000.0f, 1.0f);

    guRotate(&modeling, theta, 1.0F, 1.0F, 1.0F);
    guScale(&scale, sz, sz, sz);
    guTranslate(&translate, 0.0F, 0.0F, -10550.0F);

    gSPViewport(gDLMaster++, &vp);
    gSPClearGeometryMode(gDLMaster++, G_SHADE | G_SHADING_SMOOTH | G_CULL_BOTH |
			  G_FOG | G_LIGHTING | G_TEXTURE_GEN |
			  G_TEXTURE_GEN_LINEAR | G_LOD );

    gDPSetDepthImage(gDLMaster++, zbuffer);

    gDPSetCycleType(gDLMaster++, G_CYC_FILL);
    gDPSetColorImage(gDLMaster++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, zbuffer);
    gDPSetFillColor(gDLMaster++, GPACK_ZDZ(G_MAXFBZ,0) << 16 | GPACK_ZDZ(G_MAXFBZ,0));
    gDPFillRectangle(gDLMaster++, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);

    gDPSetCycleType(gDLMaster++, G_CYC_FILL);
    gDPSetColorImage(gDLMaster++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, framebuffer);
    gDPSetFillColor(gDLMaster++, GPACK_RGBA5551(255, 255, 255, 1) << 16 | GPACK_RGBA5551(255, 255, 255, 1));
    gDPFillRectangle(gDLMaster++, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);
    
    guPerspective(&projection, &perspNorm, 33, 320.0/240.0, 100, 2000, 1.0);
    guLookAt(&viewing,50, 100, 400, 0, 0, 0, 0, 1, 0);
}

void ViewerApp::Update() {
    this->Setup();

    gSPMatrix(gDLMaster++, &projection, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    // gSPMatrix(gDLMaster++, &modeling, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gDLMaster++, &scale, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gDLMaster++, &translate, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPPerspNormalize(gDLMaster++, perspNorm);

    gDPSetCombineMode(gDLMaster++, G_CC_MODULATEI, G_CC_MODULATEI_PRIM2);
    gSPLoadUcode(gDLMaster++, UcodeHandlers::ucode_f3dex);
    // gSPDisplayListOTRFilePath(gDLMaster++, test.c_str());
    gSPLoadUcode(gDLMaster++, UcodeHandlers::ucode_f3dex2);

    // Finish and reload
    gSPEndDisplayList(gDLMaster++);
    gDLMaster = gGfxPool;
    theta += 0.1f;
}

void ViewerApp::RunFrame() {
    GameEngine::Instance->StartFrame();

    if (GfxDebuggerIsDebugging()) {
        GameEngine::ProcessGfxCommands(&gGfxPool[0]);
        return;
    }

    Instance->Update();

    if (GfxDebuggerIsDebuggingRequested()) {
        GfxDebuggerDebugDisplayList(&gGfxPool[0]);
    }

    GameEngine::ProcessGfxCommands(&gGfxPool[0]);
}


#ifdef _WIN32
int SDL_main(int argc, char **argv) {
#else
#if defined(__cplusplus) && defined(PLATFORM_IOS)
extern "C"
#endif
int main(int argc, char *argv[]) {
#endif
    GameEngine::Create();
    Instance->Setup();
    while (WindowIsRunning()) {
        ViewerApp::RunFrame();
    }
    GameEngine::Instance->Destroy();
    return 0;
}