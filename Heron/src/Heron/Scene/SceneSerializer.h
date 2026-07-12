#pragma once

#include "Heron/Scene/Scene.h"
#include "Heron/Core/Core.h"

namespace Heron {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		void Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

	private:
		Ref<Scene> m_Scene;
	};

}
