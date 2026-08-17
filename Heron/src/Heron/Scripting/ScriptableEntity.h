#pragma once

#include "Heron/Scene/Entity.h"
#include "Heron/Scene/Components.h"

namespace Heron {

	class ScriptableEntity {
	public:
		virtual ~ScriptableEntity() = default;

		template<typename T>
		T& GetComponent() {
			return m_Entity.GetComponent<T>();
		}

		template<typename T>
		bool HasComponent() {
			return m_Entity.HasComponent<T>();
		}

	protected:
		virtual void OnCreate() {}
		virtual void OnUpdate(float dt) {}
		virtual void OnDestroy() {}

		Entity Instantiate(const std::string& name = "Entity") {
			return m_Entity.m_Scene->CreateEntity(name);
		}

		void Destroy(Entity entity) {
			m_Entity.m_Scene->DestroyEntity(entity);
		}

		Entity FindEntityByName(const std::string& name) {
			return m_Entity.m_Scene->FindEntityByName(name);
		}

		Entity GetEntity() { return m_Entity; }
		Scene* GetScene() { return m_Entity.m_Scene; }

	private:
		Entity m_Entity;
		friend class Scene;
	};

}
