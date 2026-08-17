#pragma once

#include <string>
#include <vector>

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

	struct RelationshipComponent {
		entt::entity Parent = entt::null;
		std::vector<entt::entity> Children;

		RelationshipComponent() = default;
		RelationshipComponent(const RelationshipComponent&) = default;
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
		bool FixedRotation = false;
		b3BodyId RuntimeBody = b3_nullBodyId;

		// Used to track if scripts/gizmos modified the transform this frame
		float PreviousTranslation[3] = { 0.0f, 0.0f, 0.0f };
		float PreviousRotation[3] = { 0.0f, 0.0f, 0.0f };

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

	struct LightComponent {
		float Color[3] = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;

		LightComponent() = default;
		LightComponent(const LightComponent&) = default;
	};

	enum class PrimitiveType {
		None = 0,
		Cube,
		Plane,
		Cylinder,
		Sphere,
		Custom
	};

	struct MeshComponent {
		PrimitiveType Type = PrimitiveType::Cube;
		float Color[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
		Ref<class Texture2D> Texture;
		std::string TexturePath = ""; // Keep track of path for UI
		std::string ModelPath = "";   // Keep track of custom OBJ path

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

	struct SkyboxComponent {
		std::string TexturePaths[6]; // Right, Left, Top, Bottom, Front, Back
		Ref<class TextureCube> Texture = nullptr;

		SkyboxComponent() = default;
		SkyboxComponent(const SkyboxComponent&) = default;
	};

	struct AudioComponent {
		std::vector<std::string> AudioPaths; // Playlist of audio file paths
		bool Play = false;                   // Toggle to start/stop playback
		bool Shuffle = true;                 // Shuffle mode
		bool Loop = true;                    // Loop playlist when it ends

		// Internal playback state (not serialized as settings)
		int  CurrentTrackIndex = -1;
		bool WasPlaying = false;

		AudioComponent() = default;
		AudioComponent(const AudioComponent&) = default;
	};

	struct TerrainComponent {
		std::string HeightmapPath = "";
		Ref<class Texture2D> Texture = nullptr;
		std::string TexturePath = ""; // Path for the diffuse texture
		
		float Size = 100.0f; // Width and Depth of the terrain
		float HeightScale = 20.0f; // Maximum height of the terrain
		int Resolution = 100; // Grid resolution (100x100 quads)
		float TextureTiling = 1.0f; // Texture repeat multiplier
		
		Ref<class VertexArray> Mesh = nullptr;
        std::vector<unsigned char> HeightmapData;
        int HeightmapWidth = 0;
        int HeightmapHeight = 0;
		
        std::vector<float> RuntimeHeightData;
        struct b3HeightFieldData* RuntimeHeightField = nullptr;
        b3BodyId RuntimeBody = b3_nullBodyId;

		TerrainComponent() = default;
		TerrainComponent(const TerrainComponent&) = default;
	};

}

