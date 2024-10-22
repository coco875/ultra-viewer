#include "Engine.h"
#include "Viewer.h"
#include "libultraship/bridge.h"
#include "port/ui/UIWidgets.h"
#include "DisplayList.h"
#include <libultra/gbi.h>
#include "math/matrix.h"

Gfx gGfxPool[1024];
Gfx* gDLMaster = &gGfxPool[0];
u16 framebuffer[SCREEN_HEIGHT][SCREEN_WIDTH];
u16 zbuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

static Vp vp = {
    SCREEN_WIDTH*2, SCREEN_HEIGHT*2, G_MAXZ/2, 0,	/* scale */
    SCREEN_WIDTH*2, SCREEN_HEIGHT*2, G_MAXZ/2, 0,	/* translate */
};

Lights1 sun_light = gdSPDefLights1(  80,  80,  80,  /* no ambient light */
                                   200, 200, 200,  /* white light */
                                   1,   1,   -1);
/*
  The initialization of RDP
*/
Gfx setup_rdpstate[] = {
    gsDPSetRenderMode(G_RM_OPA_SURF, G_RM_OPA_SURF2),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
    gsDPSetColorDither(G_CD_BAYER),
    gsSPEndDisplayList(),
};

/*
  The initialization of RSP
*/
Gfx setup_rspstate[] = {
    gsSPViewport(&vp),
    gsSPClearGeometryMode(0xFFFFFFFF),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK),
    gsSPTexture(0, 0, 0, 0, G_OFF),
    gsSPSetLights1(sun_light),
    gsSPEndDisplayList(),
};

float sz = 0.00003f;
float posZ = -500.0f;
Vec3f rotation = {};

ViewerApp* ViewerApp::Instance = new ViewerApp();

void ViewerApp::Load() {
    const auto mgr = Ship::Context::GetInstance()->GetResourceManager();
    const auto files = mgr->GetArchiveManager()->ListFiles("*");

    for(auto& file : *files){
        if(file == "version"){
            continue;
        }
        auto resource = mgr->LoadResourceProcess("__OTR__" + file);
        if(resource == nullptr) {
            continue;
        }
        auto metadata = resource->GetInitData();
        if(metadata == nullptr){
            continue;
        }
        if(metadata->Type == static_cast<uint32_t>(LUS::ResourceType::DisplayList)){
            this->LoadedFiles.push_back(file.c_str());
        }
    }

    std::sort(this->LoadedFiles.begin(), this->LoadedFiles.end());
}

void ViewerApp::Setup() {
    gGfxMtx = &gMainMatrixStack[0];
    gGfxMatrix = &sGfxMatrixStack[0];
    gCalcMatrix = &sCalcMatrixStack[0];

    __gSPSegment(gDLMaster++, 0, 0x0);
    __gSPDisplayList(gDLMaster++, setup_rspstate);
    __gSPDisplayList(gDLMaster++, setup_rdpstate);

    gDPSetDepthImage(gDLMaster++, zbuffer);
    gDPSetCycleType(gDLMaster++, G_CYC_FILL);
    gDPSetColorImage(gDLMaster++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, zbuffer);
    gDPSetFillColor(gDLMaster++, (GPACK_ZDZ(G_MAXFBZ,0) << 16 | GPACK_ZDZ(G_MAXFBZ,0)));
    gDPFillRectangle(gDLMaster++, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);

    gDPSetColorImage(gDLMaster++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, framebuffer);
    gDPSetFillColor(gDLMaster++, (GPACK_RGBA5551(0, 0, 0, 1) << 16 | GPACK_RGBA5551(0, 0, 0, 1)));
    gDPFillRectangle(gDLMaster++, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);
}

void ViewerApp::Update() {
    this->Setup();

    gDPSetCombineMode(gDLMaster++, G_CC_MODULATEI, G_CC_MODULATEI_PRIM2);
    if(!this->CurrentFile.empty()){
        auto resource = Ship::Context::GetInstance()->GetResourceManager()->LoadResource(this->CurrentFile);
        auto res = std::static_pointer_cast<LUS::DisplayList>(resource);

        Matrix_InitPerspective(&gDLMaster);

        Matrix_Push(&gGfxMatrix);
        Matrix_Translate(gGfxMatrix, 0.0F, 0.0F, posZ, MTXF_APPLY);
        Matrix_RotateX(gGfxMatrix, rotation.x, MTXF_APPLY);
        Matrix_RotateY(gGfxMatrix, rotation.y, MTXF_APPLY);
        Matrix_RotateZ(gGfxMatrix, rotation.z, MTXF_APPLY);
        Matrix_SetGfxMtx(&gDLMaster);

        gSPClearGeometryMode(gGfxMatrix++, 0xFFFFFFFF);
        gSPTexture(gGfxMatrix++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPSetCombineMode(gGfxMatrix++, G_CC_MODULATEI, G_CC_MODULATEI_PRIM2);
        gSPSetGeometryMode(gGfxMatrix++, G_ZBUFFER);
        gSPSetOtherMode(gGfxMatrix++, G_SETOTHERMODE_L, G_MDSFT_ALPHACOMPARE, 3, G_AC_NONE | G_ZS_PIXEL);
        gDPSetRenderMode(gGfxMatrix++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
        gDPSetPrimColor(gGfxMatrix++, 0, 0, 255, 255, 255, 255);

        gSPLoadUcode(gDLMaster++, res->UCode);
        gSPDisplayListOTRFilePath(gDLMaster++, this->CurrentFile.c_str());
        Matrix_Pop(&gGfxMatrix);
    }
    gSPLoadUcode(gDLMaster++, UcodeHandlers::ucode_f3dex2);
    gSPSetGeometryMode(gDLMaster++, G_ZBUFFER);

    // Finish and reload
    gSPEndDisplayList(gDLMaster++);
    gDLMaster = &gGfxPool[0];
}

void ViewerApp::RunFrame() {
    GameEngine::Instance->StartFrame();

    if (GfxDebuggerIsDebugging()) {
        GameEngine::RunCommands(&gGfxPool[0]);
        return;
    }

    Instance->Update();

    if (GfxDebuggerIsDebuggingRequested()) {
        GfxDebuggerDebugDisplayList(&gGfxPool[0]);
    }

    GameEngine::RunCommands(&gGfxPool[0]);
}

void ViewerApp::DrawUI() {
    ImGui::Begin("OTR Loader");
    ImGui::Text("Display Lists");
    static char filter[128] = "";
    ImGui::InputText("Filter", filter, IM_ARRAYSIZE(filter));

    if (ImGui::BeginCombo("##LFiles", this->CurrentFile.empty() ? "None" : this->CurrentFile.c_str())) {
        ImGuiListClipper clipper;
        clipper.Begin(this->LoadedFiles.size() + 1);
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                if(i == 0){
                    if (ImGui::Selectable("None", this->CurrentFile.empty())) {
                        this->CurrentFile.clear();
                    }
                } else {
                    auto path = this->LoadedFiles[i - 1];
                    if (std::strstr(path.c_str(), filter)) {  // Only show items that match the filter
                        if (ImGui::Selectable(path.c_str(), path == this->CurrentFile)) {
                            this->CurrentFile = path;
                        }
                    }
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SliderFloat("Z Pos", &posZ, -10000.0f, 10000.0f);
    ImGui::SliderFloat("X Rot", &rotation.x, -5.0f, 5.0f);
    ImGui::SliderFloat("Y Rot", &rotation.y, -5.0f, 5.0f);
    ImGui::SliderFloat("Z Rot", &rotation.z, -5.0f, 5.0f);
    ImGui::End();
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
    ViewerApp::Instance->Load();
    ViewerApp::Instance->Setup();
    while (WindowIsRunning()) {
        ViewerApp::RunFrame();
    }
    GameEngine::Instance->Destroy();
    return 0;
}