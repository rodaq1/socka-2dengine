#pragma once
#include "imgui.h"
#include "ImGuizmo.h"
#include "glm/glm.hpp"

enum class GizmoOperation {
    TRANSLATE,
    ROTATE,
    SCALE
};

enum class GizmoSpace {
    LOCAL,
    WORLD
};
extern GizmoOperation currentOperation;
extern GizmoSpace currentSpace;
