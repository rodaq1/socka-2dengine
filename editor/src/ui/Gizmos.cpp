#include "Gizmos.h"

#include "imgui.h"
#include "ImGuizmo.h"

#include <glm/gtc/type_ptr.hpp>




GizmoOperation currentOperation = GizmoOperation::TRANSLATE;
GizmoSpace currentSpace = GizmoSpace::LOCAL;