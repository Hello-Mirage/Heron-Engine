#pragma once

#include "Heron/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Heron {

	class OpenGLContext : public GraphicsContext {
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		void Init() override;
		void SwapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle;
	};

}
