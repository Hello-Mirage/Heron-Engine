#include "hrnpch.h"
#include "RendererAPI.h"

namespace Heron {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::Vulkan;
	RendererAPI::API RendererAPI::GetAPI() { return s_API; }

}
