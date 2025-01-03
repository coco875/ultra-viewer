#include "Engine.h"
#include "Viewer.h"
#include "libultraship/bridge.h"
#include "port/ui/UIWidgets.h"
#include "DisplayList.h"
#include <libultra/gbi.h>
#include "math/matrix.h"
#include "Fast3D/Fast3dWindow.h"

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
Color background = { 0.0f, 0.0f, 0.0f };
static Lights1 light;

u8 ControllerBits = 0;
ImVec2 prev = ImVec2(0.0f, 0.0f);
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
            auto res = std::static_pointer_cast<LUS::DisplayList>(resource);
            this->LoadedFiles.push_back({ file });
            this->UCodeEntries[file] = res->UCode;
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
    auto clear = GPACK_RGBA5551((u8) (background.r * 255), (u8) (background.g * 255), (u8) (background.b * 255), 1);

    __gSPSegment(gDLMaster++, 0, 0x0);
    __gSPDisplayList(gDLMaster++, setup_rspstate);
    __gSPDisplayList(gDLMaster++, setup_rdpstate);

    gDPSetDepthImage(gDLMaster++, zbuffer);
    gDPSetCycleType(gDLMaster++, G_CYC_FILL);
    gDPSetColorImage(gDLMaster++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, zbuffer);
    gDPSetFillColor(gDLMaster++, (GPACK_ZDZ(G_MAXFBZ,0) << 16 | GPACK_ZDZ(G_MAXFBZ,0)));
    gDPFillRectangle(gDLMaster++, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);

    gDPSetColorImage(gDLMaster++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, framebuffer);
    gDPSetFillColor(gDLMaster++, (clear << 16 | clear));
    gDPFillRectangle(gDLMaster++, 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);
}

void ViewerApp::ReadInput() {
    osContGetReadData(ControllerData);
    auto state = ControllerData[0];
    auto& io = ImGui::GetIO();
    const float moveSpeed = 10.0f;
    const float rotateSpeed = 0.08f;
    const ImVec2 mouse = io.MousePos;
    const float wheel = io.MouseWheel;

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

    if(ImGui::IsAnyItemHovered() || ImGui::IsAnyItemFocused()){
        return;
    }

    if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)){
        float deltaX = mouse.x - prev.x;
        float deltaY = mouse.y - prev.y;

        if(ImGui::IsKeyDown(ImGuiKey_LeftShift)){
            position.x += deltaX / 2;
            position.y -= deltaY / 2;
        } else {
            rotation.x -= deltaY * 0.008f;
            rotation.y -= deltaX * 0.008f;
        }
    }
    
    if(wheel) {
        position.z -= io.MouseWheel * 10.0;
    }

    prev = io.MousePosPrev;
}

void ViewerApp::Update() {
    this->Setup();

    Matrix_InitPerspective(&gDLMaster);

    Matrix_Push(&gGfxMatrix);
    Matrix_Scale(gGfxMatrix, scale.x, scale.y, scale.z, MTXF_APPLY);
    Matrix_Translate(gGfxMatrix, position.x, position.y, position.z, MTXF_APPLY);
    Matrix_RotateX(gGfxMatrix, rotation.x, MTXF_APPLY);
    Matrix_RotateY(gGfxMatrix, rotation.y, MTXF_APPLY);
    Matrix_RotateZ(gGfxMatrix, rotation.z, MTXF_APPLY);
    Matrix_SetGfxMtx(&gDLMaster);

    gDPSetCombineMode(gDLMaster++, G_CC_MODULATEI, G_CC_MODULATEI_PRIM2);
    gSPClearGeometryMode(gDLMaster++, 0xFFFFFFFF);
    gDPSetDepthSource(gDLMaster++, G_ZS_PIXEL);
    gDPSetRenderMode(gDLMaster++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
    gSPSetGeometryMode(gDLMaster++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_FOG | G_LIGHTING | G_CULL_BACK);

    if(UseLight){
        light = gdSPDefLights1(
            (u8) (ambient.r * 255), (u8) (ambient.g * 255), (u8) (ambient.b * 255),
            (u8) (color.r * 255), (u8) (color.g * 255), (u8) (color.b * 255),
            1,   1,   -1
        );
        gSPSetLights1(gDLMaster++, light);
    }

    for (auto& entry : this->OrderDisplay) {
        gSPLoadUcode(gDLMaster++, this->UCodeEntries[entry]);
        gSPDisplayListOTRFilePath(gDLMaster++, entry.c_str());
    }

    gSPLoadUcode(gDLMaster++, UcodeHandlers::ucode_f3dex2);
    gSPClearGeometryMode(gDLMaster++, 0xFFFFFFFF);
    Matrix_Pop(&gGfxMatrix);

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

static ImGuiTableFlags flags =
    ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
    | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
    | ImGuiTableFlags_ScrollY;

bool DrawFloatSlider(const char* text, float* value, float min, float max, float def = 0.0f) {
    bool slider = ImGui::SliderFloat(text, value, min, max);
    ImGui::SameLine();
    ImGui::PushID(text);
    if(ImGui::Button("Reset")){
        *value = def;
        slider = true;
    }
    ImGui::PopID();
    return slider;
}

void ViewerApp::DrawUI() {
    const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
    this->ReadInput();

    {
        ImGui::Begin("Display Lists");
        if (ImGui::BeginTable("files", 3, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 20))){
            ImGuiListClipper clipper;
            clipper.Begin(this->LoadedFiles.size());

            ImGui::TableSetupColumn("Draw", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 340.0f);
            ImGui::TableSetupColumn("UCode", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    auto path = this->LoadedFiles[i];
                    auto cursor = std::find(this->OrderDisplay.begin(), this->OrderDisplay.end(), path);
                    auto selected = cursor != this->OrderDisplay.end();
                    ImGui::PushID(i);
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    if(ImGui::Checkbox(("##" + path).c_str(), &selected)){
                        selected = cursor != this->OrderDisplay.end();
                        if(selected) {
                            this->OrderDisplay.erase(cursor);
                        } else {
                            this->OrderDisplay.push_back(path);
                        }
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", path.c_str());
                    ImGui::TableNextColumn();
                    switch (this->UCodeEntries[path]) {
                        case ucode_f3d:
                        case ucode_f3db:
                            ImGui::Text("F3D");
                            break;
                        case ucode_f3dex:
                        case ucode_f3dexb:
                            ImGui::Text("F3DEX");
                            break;
                        case ucode_f3dex2:
                            ImGui::Text("F3DEX2");
                            break;
                        default:
                            ImGui::Text("Unk");
                            break;
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("order", 3, flags, ImVec2(0.0f, TEXT_BASE_HEIGHT * 10))){
            ImGuiListClipper clipper;
            clipper.Begin(this->OrderDisplay.size());

            ImGui::TableSetupColumn("Order", ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Path",  ImGuiTableColumnFlags_WidthFixed, 300.0f);
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
            ImGui::TableHeadersRow();

            auto size = this->OrderDisplay.size();
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    auto path = this->OrderDisplay[i];
                    ImGui::PushID(path.c_str());
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", i);
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", path.c_str());
                    ImGui::TableNextColumn();
                    if(size > 1) {
                        ImGui::BeginDisabled((i - 1) < 0);
                        if(ImGui::SmallButton("-")){
                            auto old = this->OrderDisplay[i - 1];
                            auto nnew = this->OrderDisplay[i];

                            this->OrderDisplay[i] = old;
                            this->OrderDisplay[i - 1] = nnew;
                        }
                        ImGui::EndDisabled();
                        ImGui::SameLine();
                        ImGui::BeginDisabled((i + 1) >= size);
                        if(ImGui::SmallButton("+")){
                            auto old = this->OrderDisplay[i + 1];
                            auto nnew = this->OrderDisplay[i];

                            this->OrderDisplay[i] = old;
                            this->OrderDisplay[i + 1] = nnew;
                        }
                        ImGui::EndDisabled();
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }


    {
        ImGui::Begin("Coords");
        ImGui::SetWindowSize(ImVec2(400.0f, 400.0f), ImGuiCond_FirstUseEver);
        DrawFloatSlider("X Pos", &position.x, -2000.0f, 2000.0f);
        DrawFloatSlider("Y Pos", &position.y, -2000.0f, 2000.0f);
        DrawFloatSlider("Z Pos", &position.z, -8000.0f, 8000.0f, -500.0f);
        DrawFloatSlider("X Rot", &rotation.x, -5.0f, 5.0f);
        DrawFloatSlider("Y Rot", &rotation.y, -5.0f, 5.0f);
        DrawFloatSlider("Z Rot", &rotation.z, -5.0f, 5.0f);
        DrawFloatSlider("X Scale", &scale.x, -5.0f, 5.0f, 1.0f);
        DrawFloatSlider("Y Scale", &scale.y, -5.0f, 5.0f, 1.0f);
        DrawFloatSlider("Z Scale", &scale.z, -5.0f, 5.0f, 1.0f);
        if (DrawFloatSlider("Scale", &scale.x, -5.0f, 5.0f, 1.0f)) {
            scale.y = scale.z = scale.x;
        }
        ImGui::End();
    }

    {
        ImGui::Begin("Light");
        ImGui::SetWindowSize(ImVec2(450.0f, 900.0f), ImGuiCond_FirstUseEver);
        ImGui::Checkbox("Enable", &UseLight);
        ImGui::ColorPicker3("Ambient", (float*) &ambient);
        ImGui::ColorPicker3("Light", (float*) &color);
        ImGui::End();
    }

    {
        ImGui::Begin("Settings");
        ImGui::SetWindowSize(ImVec2(450.0f, 900.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Button("Enable SRGB")) {
            auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
            wnd->EnableSRGBMode();
        }
        ImGui::ColorPicker4("Background Color", (float*) &background);
        ImGui::End();
    }
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