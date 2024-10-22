#include "Engine.h"
#include "Viewer.h"
#include "libultraship/bridge.h"
#include "port/ui/UIWidgets.h"
#include "DisplayList.h"
#include <libultra/gbi.h>
#include "math/matrix.h"

ViewerApp* ViewerApp::Instance = new ViewerApp();

Gfx gGfxPool[1024];
Gfx* gDLMaster = &gGfxPool[0];
u16 framebuffer[SCREEN_HEIGHT][SCREEN_WIDTH];
u16 zbuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

bool UseLight = false;
Vec3f rotation = {};
Vec3f scale = { 1.0f, 1.0f, 1.0f };
Vec3f position = { 0.0f, 0.0f, -500.0f };
Color ambient = { 1.0f, 1.0f, 1.0f };
Color color   = { 1.0f, 1.0f, 1.0f };
static Lights1 light;

u8 ControllerBits = 0;
OSContPad ControllerData[MAXCONTROLLERS];
OSContStatus ControllerStatus[MAXCONTROLLERS];

static Vp vp = {
    SCREEN_WIDTH*2, SCREEN_HEIGHT*2, G_MAXZ/2, 0,	/* scale */
    SCREEN_WIDTH*2, SCREEN_HEIGHT*2, G_MAXZ/2, 0,	/* translate */
};

/*
  The initialization of RDP
*/
static Gfx setup_rdpstate[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_OPA_SURF, G_RM_OPA_SURF2),
    gsDPSetColorDither(G_CD_BAYER),
    gsDPSetScissor(G_SC_NON_INTERLACE, 0,0, SCREEN_WIDTH, SCREEN_HEIGHT),
    gsDPSetCombineMode(G_CC_MODULATERGB, G_CC_PASS2),
    gsSPEndDisplayList(),
};

/*
  The initialization of RSP
*/
static Gfx setup_rspstate[] = {
    gsSPViewport(&vp),
    gsSPClipRatio(FRUSTRATIO_2),
    gsSPClearGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_TEXTURE_GEN | G_CULL_BOTH | G_FOG | G_LIGHTING),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_CULL_BACK | G_FOG | G_LIGHTING),
    gsSPTexture(0, 0, 0, 0, G_ON),
    gsSPEndDisplayList(),
};

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

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    osContInit(NULL, &ControllerBits, &ControllerStatus[0]);
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

void ViewerApp::ReadInput() {
    osContGetReadData(ControllerData);
    auto state = ControllerData[0];
    const float moveSpeed = 10.0f;
    const float rotateSpeed = 0.08f;
    
    position.x += state.stick_x / 4;
    position.y += state.stick_y / 4;

    if(state.button & L_TRIG) {
        position.z -= moveSpeed;
    }

    if(state.button & R_TRIG) {
        position.z += moveSpeed;
    }

    if(state.button & U_CBUTTONS) {
        rotation.x -= rotateSpeed;
    }

    if(state.button & D_CBUTTONS) {
        rotation.x += rotateSpeed;
    }

    if(state.button & L_CBUTTONS) {
        rotation.y -= rotateSpeed;
    }

    if(state.button & R_CBUTTONS) {
        rotation.y += rotateSpeed;
    }

    if(state.button & A_BUTTON) {
        rotation.z += rotateSpeed;
    }

    if(state.button & B_BUTTON) {
        rotation.z -= rotateSpeed;
    }
}

void ViewerApp::Update() {
    this->ReadInput();
    this->Setup();

    gDPSetCombineMode(gDLMaster++, G_CC_MODULATEI, G_CC_MODULATEI_PRIM2);
    if(!this->CurrentFile.empty()){
        auto resource = Ship::Context::GetInstance()->GetResourceManager()->LoadResource(this->CurrentFile);
        auto res = std::static_pointer_cast<LUS::DisplayList>(resource);

        Matrix_InitPerspective(&gDLMaster);

        Matrix_Push(&gGfxMatrix);
        Matrix_Scale(gGfxMatrix, scale.x, scale.y, scale.z, MTXF_APPLY);
        Matrix_Translate(gGfxMatrix, position.x, position.y, position.z, MTXF_APPLY);
        Matrix_RotateX(gGfxMatrix, rotation.x, MTXF_APPLY);
        Matrix_RotateY(gGfxMatrix, rotation.y, MTXF_APPLY);
        Matrix_RotateZ(gGfxMatrix, rotation.z, MTXF_APPLY);
        Matrix_SetGfxMtx(&gDLMaster);

        gSPClearGeometryMode(gDLMaster++, 0xFFFFFFFF);
        gDPSetDepthSource(gDLMaster++, G_ZS_PIXEL);
        gDPSetRenderMode(gDLMaster++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
        gSPSetGeometryMode(gDLMaster++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_FOG | G_LIGHTING | G_CULL_BACK);

        if(UseLight){
            light = gdSPDefLights1(
                ambient.r * 255, ambient.g * 255, ambient.b * 255,
                color.r * 255, color.g * 255, color.b * 255,
                1,   1,   -1
            );
            gSPSetLights1(gDLMaster++, light);
        }

        gSPLoadUcode(gDLMaster++, res->UCode);
        gSPDisplayListOTRFilePath(gDLMaster++, this->CurrentFile.c_str());
        gSPLoadUcode(gDLMaster++, UcodeHandlers::ucode_f3dex2);
        gSPClearGeometryMode(gDLMaster++, 0xFFFFFFFF);
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

float LastYScroll = 0.0f;
bool ScrollWasUpdated = false;

void ViewerApp::DrawUI() {
    ImGui::Begin("OTR Loader");
    ImGui::Text("Display Lists");

    if (ImGui::BeginCombo("##LFiles", this->CurrentFile.empty() ? "None" : this->CurrentFile.c_str())) {
        ImGuiListClipper clipper;
        clipper.Begin(this->LoadedFiles.size() + 1);
        if(!ScrollWasUpdated && ImGui::GetScrollY() <= 0.0f && LastYScroll != 0.0f){
            ImGui::SetScrollY(LastYScroll);
            ScrollWasUpdated = true;
        }
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                if(i == 0){
                    if (ImGui::Selectable("None", this->CurrentFile.empty())) {
                        this->CurrentFile.clear();
                        LastYScroll = ImGui::GetScrollY();
                    }
                } else {
                    auto path = this->LoadedFiles[i - 1];
                    if (ImGui::Selectable(path.c_str(), path == this->CurrentFile)) {
                        this->CurrentFile = path;
                        LastYScroll = ImGui::GetScrollY();
                    }
                }
            }
        }
        ImGui::EndCombo();
    } else {
        ScrollWasUpdated = false;
    }

    ImGui::SliderFloat("X Pos", &position.x, -2000.0f, 2000.0f);
    ImGui::SliderFloat("Y Pos", &position.y, -2000.0f, 2000.0f);
    ImGui::SliderFloat("Z Pos", &position.z, -2000.0f, 2000.0f);
    ImGui::SliderFloat("X Rot", &rotation.x, -5.0f, 5.0f);
    ImGui::SliderFloat("Y Rot", &rotation.y, -5.0f, 5.0f);
    ImGui::SliderFloat("Z Rot", &rotation.z, -5.0f, 5.0f);
    ImGui::SliderFloat("X Scale", &scale.x, -5.0f, 5.0f);
    ImGui::SliderFloat("Y Scale", &scale.y, -5.0f, 5.0f);
    ImGui::SliderFloat("Z Scale", &scale.z, -5.0f, 5.0f);
    ImGui::End();

    ImGui::Begin("Light");
    ImGui::Checkbox("Enable", &UseLight);
    ImGui::ColorPicker3("Ambient", (float*)&ambient);
    ImGui::ColorPicker3("liGHT", (float*)&color);
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