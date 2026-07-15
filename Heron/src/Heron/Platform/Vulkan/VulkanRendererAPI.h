#pragma once

#include "Heron/Renderer/RendererAPI.h"
#include <vulkan/vulkan.h>

namespace Heron {

	class VulkanRendererAPI : public RendererAPI {
	public:
		void Init() override;
		void SetClearColor(float r, float g, float b, float a) override;
		void Clear() override;

		void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
		void DrawLines(const Ref<VertexArray>& vertexArray) override;
		void DrawLineLoop(const Ref<VertexArray>& vertexArray) override;
	};

}
