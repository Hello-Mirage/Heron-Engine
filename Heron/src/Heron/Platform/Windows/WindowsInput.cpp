#include "hrnpch.h"
#include "Heron/Core/Input.h"

#include <Windows.h>
#include <GLFW/glfw3.h>

namespace Heron {

	// Map GLFW key codes to Win32 virtual key codes
	static int GLFWKeyToVK(int glfwKey) {
		// Letters A-Z: GLFW uses ASCII (65-90), VK codes are the same
		if (glfwKey >= 65 && glfwKey <= 90) return glfwKey;
		// Digits 0-9: GLFW uses ASCII (48-57), VK codes are the same
		if (glfwKey >= 48 && glfwKey <= 57) return glfwKey;

		switch (glfwKey) {
			case GLFW_KEY_SPACE:       return VK_SPACE;
			case GLFW_KEY_ESCAPE:      return VK_ESCAPE;
			case GLFW_KEY_ENTER:       return VK_RETURN;
			case GLFW_KEY_TAB:         return VK_TAB;
			case GLFW_KEY_BACKSPACE:   return VK_BACK;
			case GLFW_KEY_INSERT:      return VK_INSERT;
			case GLFW_KEY_DELETE:      return VK_DELETE;
			case GLFW_KEY_RIGHT:       return VK_RIGHT;
			case GLFW_KEY_LEFT:        return VK_LEFT;
			case GLFW_KEY_DOWN:        return VK_DOWN;
			case GLFW_KEY_UP:          return VK_UP;
			case GLFW_KEY_PAGE_UP:     return VK_PRIOR;
			case GLFW_KEY_PAGE_DOWN:   return VK_NEXT;
			case GLFW_KEY_HOME:        return VK_HOME;
			case GLFW_KEY_END:         return VK_END;
			case GLFW_KEY_CAPS_LOCK:   return VK_CAPITAL;
			case GLFW_KEY_LEFT_SHIFT:  return VK_LSHIFT;
			case GLFW_KEY_LEFT_CONTROL:return VK_LCONTROL;
			case GLFW_KEY_LEFT_ALT:    return VK_LMENU;
			case GLFW_KEY_RIGHT_SHIFT: return VK_RSHIFT;
			case GLFW_KEY_RIGHT_CONTROL:return VK_RCONTROL;
			case GLFW_KEY_RIGHT_ALT:   return VK_RMENU;
			case GLFW_KEY_F1:          return VK_F1;
			case GLFW_KEY_F2:          return VK_F2;
			case GLFW_KEY_F3:          return VK_F3;
			case GLFW_KEY_F4:          return VK_F4;
			case GLFW_KEY_F5:          return VK_F5;
			case GLFW_KEY_F6:          return VK_F6;
			case GLFW_KEY_F7:          return VK_F7;
			case GLFW_KEY_F8:          return VK_F8;
			case GLFW_KEY_F9:          return VK_F9;
			case GLFW_KEY_F10:         return VK_F10;
			case GLFW_KEY_F11:         return VK_F11;
			case GLFW_KEY_F12:         return VK_F12;
			default:                   return glfwKey;
		}
	}

	bool Input::IsKeyPressed(int keycode) {
		int vk = GLFWKeyToVK(keycode);
		return (GetAsyncKeyState(vk) & 0x8000) != 0;
	}

	bool Input::IsMouseButtonPressed(int button) {
		int vk;
		switch (button) {
			case 0: vk = VK_LBUTTON; break;   // GLFW_MOUSE_BUTTON_LEFT
			case 1: vk = VK_RBUTTON; break;   // GLFW_MOUSE_BUTTON_RIGHT
			case 2: vk = VK_MBUTTON; break;   // GLFW_MOUSE_BUTTON_MIDDLE
			default: return false;
		}
		return (GetAsyncKeyState(vk) & 0x8000) != 0;
	}

	std::pair<float, float> Input::GetMousePosition() {
		POINT p;
		GetCursorPos(&p);
		return { (float)p.x, (float)p.y };
	}

	float Input::GetMouseX() {
		auto [x, y] = GetMousePosition();
		return x;
	}

	float Input::GetMouseY() {
		auto [x, y] = GetMousePosition();
		return y;
	}

}
