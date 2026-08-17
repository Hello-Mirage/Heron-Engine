#pragma once

#include "Heron/Core/Core.h"

namespace Heron {

	enum class CursorMode {
		Normal = 0,
		Locked = 1
	};

	class Input {
	public:
		static bool IsKeyPressed(int keycode);
		static bool IsMouseButtonPressed(int button);
		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
		static void SetCursorMode(CursorMode mode);
		static void SetMousePosition(float x, float y);

		static bool IsViewportHovered();
		static void SetViewportHovered(bool hovered);
	};

}
