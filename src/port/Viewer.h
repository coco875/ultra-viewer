#pragma once
#include "Fast3D/gfx_pc.h"

#define gSPDisplayListOTRFilePath(pkt, dl) gDma1p(pkt, G_DL_OTR_FILEPATH, dl, 0, G_DL_PUSH)

extern "C" {
    void gSPDisplayList(Gfx* pkt, Gfx* dl);
    void gSPVertex(Gfx* pkt, uintptr_t v, int n, int v0);
}

class ViewerApp {
public:
    static ViewerApp* Instance;
    void Load();
    void Setup();
    void Update();
    void ReadInput();
    void DrawUI();
    static void RunFrame();

    std::vector<std::string> LoadedFiles;
    std::vector<std::string> OrderDisplay;
    std::unordered_map<std::string, UcodeHandlers> UCodeEntries;
};