#pragma once

#include "Heron/Renderer/RendererAPI.h"

namespace Heron {

	class RenderCommand {
	public:
		static void Init();
		static void SetClearColor(float r, float g, float b, float a);
		static void Clear();
		static void DrawIndexed(const Ref<VertexArray>& vertexArray);
		static void DrawLines(const Ref<VertexArray>& vertexArray);
		static void DrawLineLoop(const Ref<VertexArray>& vertexArray);
	private:
		static RendererAPI* s_RendererAPI;
	};

}
