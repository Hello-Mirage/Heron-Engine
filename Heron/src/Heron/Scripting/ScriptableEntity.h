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

	private:
		Entity m_Entity;
		friend class Scene;
	};

}
