#pragma once
#include <libultraship/libultraship.h>

namespace AdvancedResolutionSettings {
class AdvancedResolutionSettingsWindow : public Ship::GuiWindow {
  private:
    bool IsDroppingFrames();
    std::weak_ptr<Fast::Interpreter> mInterpreter;

  protected:
    bool IsBoolArrayTrue(bool*);

  public:
    using Ship::GuiWindow::GuiWindow;

    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};
} // namespace AdvancedResolutionSettings