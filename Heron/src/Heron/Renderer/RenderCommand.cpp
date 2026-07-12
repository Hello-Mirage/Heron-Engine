#include "hrnpch.h"
#include "RenderCommand.h"
#include "Heron/Platform/OpenGL/OpenGLRendererAPI.h"

namespace Heron {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI();

}
