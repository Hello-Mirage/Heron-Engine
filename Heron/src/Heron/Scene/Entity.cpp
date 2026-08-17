#include "hrnpch.h"
#include "Entity.h"
#include "Components.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Heron {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	glm::mat4 Entity::GetWorldTransform()
	{
		if (!HasComponent<TransformComponent>())
			return glm::mat4(1.0f);

		auto& tc = GetComponent<TransformComponent>();
		
		glm::mat4 local = glm::mat4(1.0f);
		local = glm::translate(local, glm::vec3(tc.Translation[0], tc.Translation[1], tc.Translation[2]));
		local = glm::rotate(local, tc.Rotation[0], glm::vec3(1, 0, 0));
		local = glm::rotate(local, tc.Rotation[1], glm::vec3(0, 1, 0));
		local = glm::rotate(local, tc.Rotation[2], glm::vec3(0, 0, 1));
		local = glm::scale(local, glm::vec3(tc.Scale[0], tc.Scale[1], tc.Scale[2]));

		if (HasComponent<RelationshipComponent>())
		{
			auto& rc = GetComponent<RelationshipComponent>();
			if (rc.Parent != entt::null)
			{
				Entity parent{ rc.Parent, m_Scene };
				return parent.GetWorldTransform() * local;
			}
		}

		return local;
	}

}
