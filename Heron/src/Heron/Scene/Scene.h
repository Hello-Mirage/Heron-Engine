#pragma once

#include "hrnpch.h"
#include <entt/entt.hpp>
#include <box3d/box3d.h>

namespace Heron {

	class Entity;

	class Scene {
	public:
		Scene();
		~Scene();
		static Ref<Scene> Copy(Ref<Scene> other);

		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);
		Entity FindEntityByName(const std::string& name);

		void OnUpdateEditor(float deltaTime);
		void OnUpdateRuntime(float deltaTime);

		entt::registry& GetRegistry() { return m_Registry; }

	private:
		entt::registry m_Registry;
		b3WorldId m_PhysicsWorld;

		friend class Entity;
	};

}
