#include "hrnpch.h"
#include "RenderCommand.h"
#include "Heron/Platform/OpenGL/OpenGLRendererAPI.h"

namespace Heron {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI();

	void RenderCommand::Init() {
		s_RendererAPI->Init();
	}

	void RenderCommand::SetClearColor(float r, float g, float b, float a) {
		s_RendererAPI->SetClearColor(r, g, b, a);
	}

	void RenderCommand::Clear() {
		s_RendererAPI->Clear();
	}

	void RenderCommand::DrawIndexed(const Ref<VertexArray>& vertexArray) {
		s_RendererAPI->DrawIndexed(vertexArray);
	}

	void RenderCommand::DrawLines(const Ref<VertexArray>& vertexArray) {
		s_RendererAPI->DrawLines(vertexArray);
	}

	void RenderCommand::DrawLineLoop(const Ref<VertexArray>& vertexArray) {
		s_RendererAPI->DrawLineLoop(vertexArray);
	}

}
