#pragma once

#include "Heron/Core/Core.h"
#include "Heron/Events/Event.h"
#include <string>
#include <functional>

namespace Heron {

	struct WindowProps {
		std::string Title;
		uint32_t    Width;
		uint32_t    Height;

		WindowProps(const std::string& title = "Heron Engine",
		            uint32_t width = 1600,
		            uint32_t height = 900)
			: Title(title), Width(width), Height(height) {
		}
	};

	// Platform-agnostic window interface
	// Each platform provides its own implementation (WindowsWindow, etc.)
	class Window {
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth()  const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Scope<Window> Create(const WindowProps& props = WindowProps());
	};

}
