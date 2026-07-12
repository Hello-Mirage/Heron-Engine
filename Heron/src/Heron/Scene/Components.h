#pragma once

#include <string>

// EnTT
#include <entt/entt.hpp>

// Box3D
#include <box3d/box3d.h>

namespace Heron {

	struct TagComponent {
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	struct TransformComponent {
		float Translation[3] = { 0.0f, 0.0f, 0.0f };
		float Rotation[3] = { 0.0f, 0.0f, 0.0f };
		float Scale[3] = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
	};

	struct SpriteRendererComponent {
		float Color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
	};

	struct RigidBody3DComponent {
		b3BodyType Type = b3_staticBody;
		b3BodyId RuntimeBody = b3_nullBodyId;

		RigidBody3DComponent() = default;
		RigidBody3DComponent(const RigidBody3DComponent&) = default;
	};

	struct BoxCollider3DComponent {
		float HalfSize[3] = { 0.5f, 0.5f, 0.5f };
		b3BodyId RuntimeBody = b3_nullBodyId;

		BoxCollider3DComponent() = default;
		BoxCollider3DComponent(const BoxCollider3DComponent&) = default;
	};

	struct CameraComponent {
		bool Primary = true;
		float FOV = 45.0f;
		float NearClip = 0.1f;
		float FarClip = 1000.0f;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	enum class PrimitiveType {
		None = 0,
		Cube,
		Plane,
		Cylinder,
		Sphere
	};

	struct MeshComponent {
		PrimitiveType Type = PrimitiveType::Cube;
		float Color[4] = { 0.8f, 0.8f, 0.8f, 1.0f };

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(PrimitiveType type) : Type(type) {}
	};

	// Forward declaration
	class ScriptableEntity;

	struct NativeScriptComponent {
		std::string ScriptName;
		ScriptableEntity* Instance = nullptr;

		NativeScriptComponent() = default;
		NativeScriptComponent(const NativeScriptComponent& other)
			: ScriptName(other.ScriptName), Instance(nullptr) {}
	};

}
