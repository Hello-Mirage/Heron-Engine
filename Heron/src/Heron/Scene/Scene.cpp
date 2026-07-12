#include "hrnpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Heron/Scripting/ScriptableEntity.h"
#include "Heron/Scripting/ScriptRegistry.h"

namespace Heron {

	Scene::Scene() {
		b3WorldDef worldDef = b3DefaultWorldDef();
		m_PhysicsWorld = b3CreateWorld(&worldDef);
	}

	Scene::~Scene() {
		b3DestroyWorld(m_PhysicsWorld);
	}

	template<typename Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<entt::entity, entt::entity>& enttMap)
	{
		auto view = src.view<Component>();
		for (auto e : view)
		{
			entt::entity dstEnttID = enttMap.at(e);
			auto& component = src.get<Component>(e);
			dst.emplace_or_replace<Component>(dstEnttID, component);
		}
	}

	Ref<Scene> Scene::Copy(Ref<Scene> other) {
		Ref<Scene> newScene = CreateRef<Scene>();
		
		std::unordered_map<entt::entity, entt::entity> enttMap;
		
		auto& srcRegistry = other->m_Registry;
		auto& dstRegistry = newScene->m_Registry;
		
		auto idView = srcRegistry.view<TagComponent>();
		for (auto e : idView)
		{
			auto& tag = srcRegistry.get<TagComponent>(e).Tag;
			Entity newEntity = newScene->CreateEntity(tag);
			enttMap[e] = (entt::entity)newEntity;
		}
		
		CopyComponent<TransformComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<SpriteRendererComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<MeshComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<RigidBody3DComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<BoxCollider3DComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<NativeScriptComponent>(dstRegistry, srcRegistry, enttMap);

		// Reset runtime physics bodies in the new scene
		auto rbView = dstRegistry.view<RigidBody3DComponent>();
		for (auto e : rbView) {
			auto& rb = dstRegistry.get<RigidBody3DComponent>(e);
			rb.RuntimeBody = b3_nullBodyId;
		}
		auto boxView = dstRegistry.view<BoxCollider3DComponent>();
		for (auto e : boxView) {
			auto& box = dstRegistry.get<BoxCollider3DComponent>(e);
			box.RuntimeBody = b3_nullBodyId;
		}

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name) {
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity) {
		// Clean up script instance if present
		if (m_Registry.any_of<NativeScriptComponent>((entt::entity)entity)) {
			auto& nsc = m_Registry.get<NativeScriptComponent>((entt::entity)entity);
			if (nsc.Instance) {
				nsc.Instance->OnDestroy();
				delete nsc.Instance;
				nsc.Instance = nullptr;
			}
		}
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdateEditor(float deltaTime) {
		// No physics step in editor mode
	}

	void Scene::OnUpdateRuntime(float deltaTime) {
		// === Script Update ===
		{
			auto scriptView = m_Registry.view<NativeScriptComponent>();
			for (auto e : scriptView) {
				auto& nsc = scriptView.get<NativeScriptComponent>(e);

				if (!nsc.Instance && !nsc.ScriptName.empty()) {
					nsc.Instance = ScriptRegistry::Create(nsc.ScriptName);
					if (nsc.Instance) {
						nsc.Instance->m_Entity = Entity{ e, this };
						nsc.Instance->OnCreate();
					}
				}

				if (nsc.Instance)
					nsc.Instance->OnUpdate(deltaTime);
			}
		}

		// === Physics Step ===
		{
			// Instantiate bodies if not created
			auto view = m_Registry.view<RigidBody3DComponent, TransformComponent>();
			for (auto e : view) {
				auto [rb, transform] = view.get<RigidBody3DComponent, TransformComponent>(e);

				if (B3_IS_NULL(rb.RuntimeBody)) {
					b3BodyDef bodyDef = b3DefaultBodyDef();
					bodyDef.type = rb.Type;
					bodyDef.position = { transform.Translation[0], transform.Translation[1], transform.Translation[2] };
					
					// Set rotation from euler (roll, pitch, yaw)
					b3Quat rot = b3MakeQuatFromAxisAngle({1, 0, 0}, transform.Rotation[0]);
					rot = b3MulQuat(rot, b3MakeQuatFromAxisAngle({0, 1, 0}, transform.Rotation[1]));
					rot = b3MulQuat(rot, b3MakeQuatFromAxisAngle({0, 0, 1}, transform.Rotation[2]));
					bodyDef.rotation = rot;

					rb.RuntimeBody = b3CreateBody(m_PhysicsWorld, &bodyDef);

					// Add Box Collider if present
					if (m_Registry.all_of<BoxCollider3DComponent>(e)) {
						auto& box = m_Registry.get<BoxCollider3DComponent>(e);
						b3ShapeDef shapeDef = b3DefaultShapeDef();
						shapeDef.density = 1.0f;
						b3BoxHull boxHull = b3MakeBoxHull(box.HalfSize[0] * transform.Scale[0], 
						                                  box.HalfSize[1] * transform.Scale[1], 
						                                  box.HalfSize[2] * transform.Scale[2]);
						b3CreateHullShape(rb.RuntimeBody, &shapeDef, &boxHull.base);
						box.RuntimeBody = rb.RuntimeBody;
					}
				} else {
					// Body already exists - sync transform changes made by scripts this frame TO the physics engine
					b3Pos pos = { transform.Translation[0], transform.Translation[1], transform.Translation[2] };
					
					b3Quat rot = b3MakeQuatFromAxisAngle({1, 0, 0}, transform.Rotation[0]);
					rot = b3MulQuat(rot, b3MakeQuatFromAxisAngle({0, 1, 0}, transform.Rotation[1]));
					rot = b3MulQuat(rot, b3MakeQuatFromAxisAngle({0, 0, 1}, transform.Rotation[2]));

					b3Body_SetTransform(rb.RuntimeBody, pos, rot);
				}
			}

			// Instantiate bodies for standalone colliders (static bodies)
			auto viewBox = m_Registry.view<BoxCollider3DComponent, TransformComponent>(entt::exclude<RigidBody3DComponent>);
			for (auto e : viewBox) {
				auto [box, transform] = viewBox.get<BoxCollider3DComponent, TransformComponent>(e);

				if (B3_IS_NULL(box.RuntimeBody)) {
					b3BodyDef bodyDef = b3DefaultBodyDef();
					bodyDef.type = b3_staticBody;
					bodyDef.position = { transform.Translation[0], transform.Translation[1], transform.Translation[2] };
					
					b3Quat rot = b3MakeQuatFromAxisAngle({1, 0, 0}, transform.Rotation[0]);
					rot = b3MulQuat(rot, b3MakeQuatFromAxisAngle({0, 1, 0}, transform.Rotation[1]));
					rot = b3MulQuat(rot, b3MakeQuatFromAxisAngle({0, 0, 1}, transform.Rotation[2]));
					bodyDef.rotation = rot;

					box.RuntimeBody = b3CreateBody(m_PhysicsWorld, &bodyDef);
					
					b3ShapeDef shapeDef = b3DefaultShapeDef();
					b3BoxHull boxHull = b3MakeBoxHull(box.HalfSize[0] * transform.Scale[0], 
					                                  box.HalfSize[1] * transform.Scale[1], 
					                                  box.HalfSize[2] * transform.Scale[2]);
					b3CreateHullShape(box.RuntimeBody, &shapeDef, &boxHull.base);
				}
			}

			// Step Simulation
			int subStepCount = 4;
			b3World_Step(m_PhysicsWorld, deltaTime, subStepCount);

			// Retrieve transforms from Physics engine
			for (auto e : view) {
				auto [rb, transform] = view.get<RigidBody3DComponent, TransformComponent>(e);
				if (!B3_IS_NULL(rb.RuntimeBody) && rb.Type == b3_dynamicBody) {
					b3Vec3 pos = b3Body_GetPosition(rb.RuntimeBody);
					transform.Translation[0] = pos.x;
					transform.Translation[1] = pos.y;
					transform.Translation[2] = pos.z;

					b3Quat rot = b3Body_GetRotation(rb.RuntimeBody);
					// Convert quaternion (v.x, v.y, v.z, s) to Euler angles (roll, pitch, yaw)
					float qx = rot.v.x, qy = rot.v.y, qz = rot.v.z, qw = rot.s;

					float sinr_cosp = 2.0f * (qw * qx + qy * qz);
					float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
					transform.Rotation[0] = atan2f(sinr_cosp, cosr_cosp);

					float sinp = 2.0f * (qw * qy - qz * qx);
					transform.Rotation[1] = (fabsf(sinp) >= 1.0f) ? copysignf(3.14159265f / 2.0f, sinp) : asinf(sinp);

					float siny_cosp = 2.0f * (qw * qz + qx * qy);
					float cosy_cosp = 1.0f - 2.0f * (qy * qy + qz * qz);
					transform.Rotation[2] = atan2f(siny_cosp, cosy_cosp);
				}
			}
		}
	}

}
