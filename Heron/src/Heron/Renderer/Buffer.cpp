#include "hrnpch.h"
#include "Buffer.h"

#include "Heron/Platform/OpenGL/OpenGLBuffer.h"

namespace Heron {

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size) {
		return CreateRef<OpenGLVertexBuffer>(vertices, size);
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count) {
		return CreateRef<OpenGLIndexBuffer>(indices, count);
	}

}
