#include "Engine.h"
#include "Viewer.h"
#include "libultraship/bridge.h"
#include "port/ui/UIWidgets.h"
#include "DisplayList.h"
#include <libultra/gbi.h>
#include "math/matrix.h"

Gfx gGfxPool[1024];
Gfx* gDLMaster = &gGfxPool[0];
u16 framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
u16 zbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

static Vp vp = {
    SCREEN_WIDTH*2, SCREEN_HEIGHT*2, G_MAXZ/2, 0,	/* scale */
    SCREEN_WIDTH*2, SCREEN_HEIGHT*2, G_MAXZ/2, 0,	/* translate */
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
}

void ViewerApp::Setup() {
    gGfxMtx = &gMainMatrixStack[0];
    gGfxMatrix = &sGfxMatrixStack[0];
    gCalcMatrix = &sCalcMatrixStack[0];

    Matrix_InitPerspective(&gDLMaster);
    gSPViewport(gDLMaster++, &vp);

    gDPSetDepthImage(gDLMaster++, &zbuffer);
    gDPSetRenderMode(gDLMaster++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetColorImage(gDLMaster++, G_IM_FMT_RGBA, G_IM_SIZ_16b, OTRGetRectDimensionFromRightEdge(SCREEN_WIDTH), &zbuffer);
    gDPSetFillColor(gDLMaster++, FILL_COLOR(GPACK_ZDZ(G_MAXFBZ, 0)));
    gDPFillRectangle(gDLMaster++, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);

    gDPSetCycleType(gDLMaster++, G_CYC_FILL);
    gDPSetColorImage(gDLMaster++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, framebuffer);
    gDPSetFillColor(gDLMaster++, GPACK_RGBA5551(255, 255, 255, 1) << 16 | GPACK_RGBA5551(255, 255, 255, 1));
    gDPFillRectangle(gDLMaster++, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);
}

void ViewerApp::Update() {
    this->Setup();

    gDPSetCombineMode(gDLMaster++, G_CC_MODULATEI, G_CC_MODULATEI_PRIM2);
    if(!this->CurrentFile.empty()){
        auto resource = Ship::Context::GetInstance()->GetResourceManager()->LoadResource(this->CurrentFile);
        auto res = std::static_pointer_cast<LUS::DisplayList>(resource);

        Matrix_Push(&gGfxMatrix);
        Matrix_Translate(gGfxMatrix, 0.0F, 0.0F, posZ, MTXF_APPLY);
        Matrix_RotateX(gGfxMatrix, rotation.x, MTXF_APPLY);
        Matrix_RotateY(gGfxMatrix, rotation.y, MTXF_APPLY);
        Matrix_RotateZ(gGfxMatrix, rotation.z, MTXF_APPLY);
        Matrix_SetGfxMtx(&gDLMaster);
        gSPSetGeometryMode(gDLMaster++, G_ZBUFFER | G_CULL_BACK);
        gDPSetRenderMode(gDLMaster++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        gSPSetGeometryMode(gDLMaster++, G_CULL_BACK);

        gSPLoadUcode(gDLMaster++, res->UCode);
        gSPDisplayListOTRFilePath(gDLMaster++, this->CurrentFile.c_str());
        Matrix_Pop(&gGfxMatrix);
    }
    gSPLoadUcode(gDLMaster++, UcodeHandlers::ucode_f3dex2);
    gSPSetGeometryMode(gDLMaster++, G_ZBUFFER);

    // Finish and reload
    gSPEndDisplayList(gDLMaster++);
    gDLMaster = &gGfxPool[0];
//    theta += 0.1f;
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
    if (ImGui::BeginCombo("##LFiles", this->CurrentFile.empty() ? "None" : this->CurrentFile.c_str())) {
        if (ImGui::Selectable("None", this->CurrentFile.empty())) {
            this->CurrentFile.clear();
        }

        for (uint8_t i = 0; i < MIN(this->LoadedFiles.size(), 20); i++) {
            auto path = this->LoadedFiles[i];
            if (ImGui::Selectable(path.c_str(), path == this->CurrentFile)) {
                this->CurrentFile = path;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SliderFloat("Z Pos", &posZ, -10000.0f, 10000.0f);
    ImGui::SliderFloat("X Rot", &rotation.x, -1.0f, 1.0f);
    ImGui::SliderFloat("Y Rot", &rotation.y, -1.0f, 1.0f);
    ImGui::SliderFloat("Z Rot", &rotation.z, -1.0f, 1.0f);
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