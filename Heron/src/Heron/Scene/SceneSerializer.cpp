#include "hrnpch.h"
#include "SceneSerializer.h"
#include "Entity.h"
#include "Components.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace YAML {
	template<>
	struct convert<float[3]> {
		static Node encode(const float rhs[3]) {
			Node node;
			node.push_back(rhs[0]);
			node.push_back(rhs[1]);
			node.push_back(rhs[2]);
			return node;
		}
		static bool decode(const Node& node, float rhs[3]) {
			if (!node.IsSequence() || node.size() != 3) return false;
			rhs[0] = node[0].as<float>();
			rhs[1] = node[1].as<float>();
			rhs[2] = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<float[4]> {
		static Node encode(const float rhs[4]) {
			Node node;
			node.push_back(rhs[0]);
			node.push_back(rhs[1]);
			node.push_back(rhs[2]);
			node.push_back(rhs[3]);
			return node;
		}
		static bool decode(const Node& node, float rhs[4]) {
			if (!node.IsSequence() || node.size() != 4) return false;
			rhs[0] = node[0].as<float>();
			rhs[1] = node[1].as<float>();
			rhs[2] = node[2].as<float>();
			rhs[3] = node[3].as<float>();
			return true;
		}
	};
}

namespace Heron {

	#define WRITE_VEC3(out, name, v) \
		out << YAML::Key << name << YAML::Value << YAML::Flow << YAML::BeginSeq << v[0] << v[1] << v[2] << YAML::EndSeq

	#define WRITE_VEC4(out, name, v) \
		out << YAML::Key << name << YAML::Value << YAML::Flow << YAML::BeginSeq << v[0] << v[1] << v[2] << v[3] << YAML::EndSeq

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << (uint32_t)entity; // TODO: UUIDs

		if (entity.HasComponent<TagComponent>()) {
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap;
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<TransformComponent>()) {
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap;
			auto& tc = entity.GetComponent<TransformComponent>();
			WRITE_VEC3(out, "Translation", tc.Translation);
			WRITE_VEC3(out, "Rotation", tc.Rotation);
			WRITE_VEC3(out, "Scale", tc.Scale);
			out << YAML::EndMap;
		}

		if (entity.HasComponent<MeshComponent>()) {
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap;
			auto& mc = entity.GetComponent<MeshComponent>();
			out << YAML::Key << "Type" << YAML::Value << (int)mc.Type;
			WRITE_VEC4(out, "Color", mc.Color);
			out << YAML::EndMap;
		}

		if (entity.HasComponent<RigidBody3DComponent>()) {
			out << YAML::Key << "RigidBody3DComponent";
			out << YAML::BeginMap;
			auto& rb = entity.GetComponent<RigidBody3DComponent>();
			out << YAML::Key << "Type" << YAML::Value << (int)rb.Type;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<BoxCollider3DComponent>()) {
			out << YAML::Key << "BoxCollider3DComponent";
			out << YAML::BeginMap;
			auto& bc = entity.GetComponent<BoxCollider3DComponent>();
			WRITE_VEC3(out, "HalfSize", bc.HalfSize);
			out << YAML::EndMap;
		}

		if (entity.HasComponent<NativeScriptComponent>()) {
			out << YAML::Key << "NativeScriptComponent";
			out << YAML::BeginMap;
			auto& nsc = entity.GetComponent<NativeScriptComponent>();
			out << YAML::Key << "ScriptName" << YAML::Value << nsc.ScriptName;
			out << YAML::EndMap;
		}

		if (entity.HasComponent<CameraComponent>()) {
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap;
			auto& cc = entity.GetComponent<CameraComponent>();
			out << YAML::Key << "Primary" << YAML::Value << cc.Primary;
			out << YAML::Key << "FOV" << YAML::Value << cc.FOV;
			out << YAML::Key << "NearClip" << YAML::Value << cc.NearClip;
			out << YAML::Key << "FarClip" << YAML::Value << cc.FarClip;
			out << YAML::EndMap;
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		m_Scene->GetRegistry().each([&](auto entityID) {
			Entity entity = { entityID, m_Scene.get() };
			if (!entity) return;
			SerializeEntity(out, entity);
		});

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		HRN_CORE_INFO("Deserializing scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities) {
			for (auto entity : entities) {
				uint32_t uuid = entity["Entity"].as<uint32_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				Entity deserializedEntity = m_Scene->CreateEntity(name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent) {
					// Transform component is added by default on creation
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation[0] = transformComponent["Translation"][0].as<float>();
					tc.Translation[1] = transformComponent["Translation"][1].as<float>();
					tc.Translation[2] = transformComponent["Translation"][2].as<float>();

					tc.Rotation[0] = transformComponent["Rotation"][0].as<float>();
					tc.Rotation[1] = transformComponent["Rotation"][1].as<float>();
					tc.Rotation[2] = transformComponent["Rotation"][2].as<float>();

					tc.Scale[0] = transformComponent["Scale"][0].as<float>();
					tc.Scale[1] = transformComponent["Scale"][1].as<float>();
					tc.Scale[2] = transformComponent["Scale"][2].as<float>();
				}

				auto meshComponent = entity["MeshComponent"];
				if (meshComponent) {
					auto& mc = deserializedEntity.AddComponent<MeshComponent>();
					mc.Type = (PrimitiveType)meshComponent["Type"].as<int>();
					
					mc.Color[0] = meshComponent["Color"][0].as<float>();
					mc.Color[1] = meshComponent["Color"][1].as<float>();
					mc.Color[2] = meshComponent["Color"][2].as<float>();
					mc.Color[3] = meshComponent["Color"][3].as<float>();
				}

				auto rigidBodyComponent = entity["RigidBody3DComponent"];
				if (rigidBodyComponent) {
					auto& rb = deserializedEntity.AddComponent<RigidBody3DComponent>();
					rb.Type = (b3BodyType)rigidBodyComponent["Type"].as<int>();
				}

				auto boxColliderComponent = entity["BoxCollider3DComponent"];
				if (boxColliderComponent) {
					auto& bc = deserializedEntity.AddComponent<BoxCollider3DComponent>();
					bc.HalfSize[0] = boxColliderComponent["HalfSize"][0].as<float>();
					bc.HalfSize[1] = boxColliderComponent["HalfSize"][1].as<float>();
					bc.HalfSize[2] = boxColliderComponent["HalfSize"][2].as<float>();
				}

				auto nativeScriptComponent = entity["NativeScriptComponent"];
				if (nativeScriptComponent) {
					auto& nsc = deserializedEntity.AddComponent<NativeScriptComponent>();
					nsc.ScriptName = nativeScriptComponent["ScriptName"].as<std::string>();
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent) {
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();
					cc.Primary = cameraComponent["Primary"].as<bool>();
					cc.FOV = cameraComponent["FOV"].as<float>();
					cc.NearClip = cameraComponent["NearClip"].as<float>();
					cc.FarClip = cameraComponent["FarClip"].as<float>();
				}
			}
		}

		return true;
	}

}
