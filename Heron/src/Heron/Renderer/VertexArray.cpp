#include "hrnpch.h"
#include "VertexArray.h"

#include "Heron/Platform/OpenGL/OpenGLVertexArray.h"

namespace Heron {

	Ref<VertexArray> VertexArray::Create() {
		return CreateRef<OpenGLVertexArray>();
	}

}
