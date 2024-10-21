#include "Engine.h"

#include "DisplayListFactory.h"
#include "MatrixFactory.h"
#include "TextureFactory.h"
#include "LightFactory.h"
#include "VertexFactory.h"
#include "StringHelper.h"
#include "Fast3D/Fast3dWindow.h"
#include "ui/ImguiUI.h"
#include <Fast3D/gfx_pc.h>

GameEngine* GameEngine::Instance;

GameEngine::GameEngine() {
    std::vector<std::string> DataFiles;
    const std::string files = Ship::Context::GetPathRelativeToAppDirectory(".");

    if (!files.empty() && std::filesystem::exists(files)) {
        if (std::filesystem::is_directory(files)) {
            for (const auto&p: std::filesystem::recursive_directory_iterator(files)) {
                auto ext = p.path().extension().string();
                if (StringHelper::IEquals(ext, ".otr") || StringHelper::IEquals(ext, ".o2r")) {
                    DataFiles.push_back(p.path().generic_string());
                }
            }
        }
    }

    this->context = Ship::Context::CreateInstance("Ultra Viewer 64", "uv64", "uviewer.cfg.json", DataFiles, {}, 3, { 32000, 1024, 2480 });
    auto loader = context->GetResourceManager()->GetResourceLoader();
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY, "Texture", static_cast<uint32_t>(LUS::ResourceType::Texture), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryTextureV1>(), RESOURCE_FORMAT_BINARY, "Texture", static_cast<uint32_t>(LUS::ResourceType::Texture), 1);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryVertexV0>(), RESOURCE_FORMAT_BINARY, "Vertex", static_cast<uint32_t>(LUS::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryDisplayListV0>(), RESOURCE_FORMAT_BINARY, "DisplayList", static_cast<uint32_t>(LUS::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryMatrixV0>(), RESOURCE_FORMAT_BINARY, "Matrix", static_cast<uint32_t>(LUS::ResourceType::Matrix), 0);
    loader->RegisterResourceFactory(std::make_shared<LUS::ResourceFactoryBinaryLightV0>(), RESOURCE_FORMAT_BINARY, "Light", static_cast<uint32_t>(LUS::ResourceType::Light), 0);
}

void GameEngine::Create(){
    Instance = new GameEngine();
    GameUI::SetupGuiElements();
}

void GameEngine::Destroy(){

}

bool ShouldClearTextureCacheAtEndOfFrame = false;

void GameEngine::StartFrame() const{
    using Ship::KbScancode;
    const int32_t dwScancode = this->context->GetWindow()->GetLastScancode();
    this->context->GetWindow()->SetLastScancode(-1);

    // TODO: Handle inputs
    this->context->GetWindow()->StartFrame();
}

void GameEngine::RunCommands(Gfx* Commands) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    wnd->SetTargetFps(60);
    wnd->SetMaximumFrameLatency(1);
    wnd->SetRendererUCode(UcodeHandlers::ucode_f3dex2);

    gfx_run(Commands, {});
    gfx_end_frame();

    if (ShouldClearTextureCacheAtEndOfFrame) {
        gfx_texture_cache_clear();
        ShouldClearTextureCacheAtEndOfFrame = false;
    }
}

// End

float GameEngine_GetAspectRatio() {
    return gfx_current_dimensions.aspect_ratio;
}

float OTRGetDimensionFromLeftEdge(float v) {
    return (gfx_native_dimensions.width / 2 - gfx_native_dimensions.height / 2 * GameEngine_GetAspectRatio() + (v));
}

float OTRGetDimensionFromRightEdge(float v) {
    return (gfx_native_dimensions.width / 2 + gfx_native_dimensions.height / 2 * GameEngine_GetAspectRatio() -
            (gfx_native_dimensions.width - v));
}

uint32_t OTRGetGameRenderWidth() {
    return gfx_current_dimensions.width;
}

uint32_t OTRGetGameRenderHeight() {
    return gfx_current_dimensions.height;
}

int16_t OTRGetRectDimensionFromLeftEdge(float v) {
    return ((int)floorf(OTRGetDimensionFromLeftEdge(v)));
}

int16_t OTRGetRectDimensionFromRightEdge(float v) {
    return ((int)ceilf(OTRGetDimensionFromRightEdge(v)));
}

static const char* sOtrSignature = "__OTR__";

uint8_t GameEngine_OTRSigCheck(const char* data) {
    if(data == nullptr) {
        return 0;
    }
    return strncmp(data, sOtrSignature, strlen(sOtrSignature)) == 0;
}