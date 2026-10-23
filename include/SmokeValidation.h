#pragma once
#include "ApplicationInterface.h"
struct GLFWwindow;
namespace validation {
// Same original keyboard/snapshot checks plus real ImGui event-driven widgets.
void keyboardStep(int frame, GLFWwindow* window, city::CandidateApplication& app);
void mouseStep(int frame, const ui::ApplicationInterface& interface);
void checkFrame(int frame, const city::CandidateApplication& app, ui::Rect viewport,
                int framebufferWidth, int framebufferHeight, float scaleX, float scaleY);
inline constexpr int smokeFrames = 66;
}
