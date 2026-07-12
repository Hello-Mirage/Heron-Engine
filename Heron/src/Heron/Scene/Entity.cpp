#include "hrnpch.h"
#include "Entity.h"

namespace Heron {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

}
