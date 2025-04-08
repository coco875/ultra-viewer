#include "Engine.h"

#include "DisplayListFactory.h"
#include "MatrixFactory.h"
#include "TextureFactory.h"
#include "LightFactory.h"
#include "VertexFactory.h"
#include "StringHelper.h"
#include "Fast3D/Fast3dWindow.h"
#include "ui/ImguiUI.h"
#include <Fast3D/interpreter.h>

GameEngine* GameEngine::Instance;

GameEngine::GameEngine() {
    std::vector<std::string> DataFiles;
    const std::string files = Ship::Context::GetPathRelativeToAppDirectory(".");

    if (!files.empty() && std::filesystem::exists(files)) {
        if (std::filesystem::is_directory(files)) {
            for (const auto& p : std::filesystem::recursive_directory_iterator(files)) {
                auto ext = p.path().extension().string();
                if (StringHelper::IEquals(ext, ".otr") || StringHelper::IEquals(ext, ".o2r")) {
                    DataFiles.push_back(p.path().generic_string());
                }
            }
        }
    }

    this->context = Ship::Context::CreateUninitializedInstance("Ultra Viewer 64", "uv64", "uviewer.cfg.json");

    this->context->InitConfiguration();    // without this line InitConsoleVariables fails at Config::Reload()
    this->context->InitConsoleVariables(); // without this line the controldeck constructor failes in
                                           // ShipDeviceIndexMappingManager::UpdateControllerNamesFromConfig()

    auto controlDeck = std::make_shared<LUS::ControlDeck>();

    this->context->InitResourceManager(DataFiles, {}, 3); // without this line InitWindow fails in Gui::Init()
    this->context->InitConsole(); // without this line the GuiWindow constructor fails in ConsoleWindow::InitElement()

    auto wnd = std::make_shared<Fast::Fast3dWindow>(std::vector<std::shared_ptr<Ship::GuiWindow>>({}));
    // auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());

    this->context->Init(DataFiles, {}, 3, { 32000, 1024, 2480 }, wnd, controlDeck);

    // this->context = Ship::Context::CreateInstance("Ultra Viewer 64", "uv64", "uviewer.cfg.json", DataFiles, {}, 3, {
    // 32000, 1024, 2480 });

    auto loader = context->GetResourceManager()->GetResourceLoader();
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV0>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryTextureV1>(), RESOURCE_FORMAT_BINARY,
                                    "Texture", static_cast<uint32_t>(Fast::ResourceType::Texture), 1);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryVertexV0>(), RESOURCE_FORMAT_BINARY,
                                    "Vertex", static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryXMLVertexV0>(), RESOURCE_FORMAT_XML, "Vertex",
                                    static_cast<uint32_t>(Fast::ResourceType::Vertex), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryDisplayListV0>(),
                                    RESOURCE_FORMAT_BINARY, "DisplayList",
                                    static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryXMLDisplayListV0>(), RESOURCE_FORMAT_XML,
                                    "DisplayList", static_cast<uint32_t>(Fast::ResourceType::DisplayList), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryMatrixV0>(), RESOURCE_FORMAT_BINARY,
                                    "Matrix", static_cast<uint32_t>(Fast::ResourceType::Matrix), 0);
    loader->RegisterResourceFactory(std::make_shared<Fast::ResourceFactoryBinaryLightV0>(), RESOURCE_FORMAT_BINARY,
                                    "Light", static_cast<uint32_t>(Fast::ResourceType::Light), 0);
}

void GameEngine::Create() {
    Instance = new GameEngine();
    GameUI::SetupGuiElements();
}

void GameEngine::Destroy() {
}

bool ShouldClearTextureCacheAtEndOfFrame = false;

void GameEngine::StartFrame() const {
    using Ship::KbScancode;
    const int32_t dwScancode = this->context->GetWindow()->GetLastScancode();
    this->context->GetWindow()->SetLastScancode(-1);

    // TODO: Handle inputs
}

void GameEngine::RunCommands(Gfx* Commands) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    wnd->SetTargetFps(60);
    wnd->SetMaximumFrameLatency(1);
    wnd->SetRendererUCode(UcodeHandlers::ucode_f3dex2);

    // Process window events for resize, mouse, keyboard events
    wnd->HandleEvents();

    wnd->DrawAndRunGraphicsCommands(Commands, {});

    if (ShouldClearTextureCacheAtEndOfFrame) {
        gfx_texture_cache_clear();
        ShouldClearTextureCacheAtEndOfFrame = false;
    }
}

// End

float GameEngine_GetAspectRatio() {
    return Ship::Context::GetInstance()->GetWindow()->GetAspectRatio();
}

float OTRGetDimensionFromLeftEdge(float v) {
    Fast::Interpreter* gfx = static_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow())
                                 ->GetInterpreterWeak()
                                 .lock()
                                 .get();
    auto gfx_native_dimensions = gfx->mNativeDimensions;
    return (gfx_native_dimensions.width / 2 - gfx_native_dimensions.height / 2 * GameEngine_GetAspectRatio() + (v));
}

float OTRGetDimensionFromRightEdge(float v) {
    Fast::Interpreter* gfx = static_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow())
                                 ->GetInterpreterWeak()
                                 .lock()
                                 .get();
    auto gfx_native_dimensions = gfx->mNativeDimensions;
    return (gfx_native_dimensions.width / 2 + gfx_native_dimensions.height / 2 * GameEngine_GetAspectRatio() -
            (gfx_native_dimensions.width - v));
}

uint32_t OTRGetGameRenderWidth() {
    return Ship::Context::GetInstance()->GetWindow()->GetWidth();
}

uint32_t OTRGetGameRenderHeight() {
    return Ship::Context::GetInstance()->GetWindow()->GetHeight();
}

int16_t OTRGetRectDimensionFromLeftEdge(float v) {
    return ((int) floorf(OTRGetDimensionFromLeftEdge(v)));
}

int16_t OTRGetRectDimensionFromRightEdge(float v) {
    return ((int) ceilf(OTRGetDimensionFromRightEdge(v)));
}

static const char* sOtrSignature = "__OTR__";

uint8_t GameEngine_OTRSigCheck(const char* data) {
    if (data == nullptr) {
        return 0;
    }
    return strncmp(data, sOtrSignature, strlen(sOtrSignature)) == 0;
}