#pragma once
#include "CandidateApplication.h"
#include <map>
#include <string>
#include <functional>

namespace ui {
// Logical window coordinates (top-left origin), converted to framebuffer pixels in main.
struct Rect { float x, y, width, height; };
class ApplicationInterface {
public:
    void initializeStyle();
    void applyPendingAction();
    Rect draw(city::CandidateApplication& app, float width, float height);
    // Actual widget hit areas, also used by automated mouse interaction checks.
    std::map<std::string,Rect> targets;
    std::string error;
private:
    std::function<void()> pendingAction_;
    void remember(const std::string& name);
};
} // namespace ui
