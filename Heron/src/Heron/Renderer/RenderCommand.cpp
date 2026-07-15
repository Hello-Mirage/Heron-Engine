#include "hrnpch.h"
#include "RenderCommand.h"
#include "Heron/Platform/OpenGL/OpenGLRendererAPI.h"
#include "Heron/Platform/Vulkan/VulkanRendererAPI.h"

namespace Heron {

	RendererAPI* RenderCommand::s_RendererAPI = nullptr;

	void RenderCommand::Init() {
		Heron::Log::Init();
		switch (RendererAPI::GetAPI()) {
			case RendererAPI::API::OpenGL: s_RendererAPI = new OpenGLRendererAPI(); break;
			case RendererAPI::API::Vulkan: s_RendererAPI = new VulkanRendererAPI(); break;
			default: HRN_CORE_ASSERT(false, "Unknown RendererAPI"); break;
		}
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
