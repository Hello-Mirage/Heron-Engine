#include "hrnpch.h"
#include "OpenGLContext.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace Heron {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle) {
		HRN_CORE_ASSERT(windowHandle, "Window handle is null!");
	}

	void OpenGLContext::Init() {
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
		HRN_CORE_ASSERT(status, "Failed to initialize Glad!");

		HRN_CORE_INFO("OpenGL Info:");
		HRN_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
		HRN_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
		HRN_CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));
	}

	void OpenGLContext::SwapBuffers() {
		glfwSwapBuffers(m_WindowHandle);
	}

}
