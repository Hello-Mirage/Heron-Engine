#include "hrnpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components.h"
#include "Heron/Scripting/ScriptableEntity.h"
#include "Heron/Scripting/ScriptRegistry.h"
#include "Heron/Audio/AudioSystem.h"
#include <algorithm>
#include <random>


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

	template<typename Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		if (src.HasComponent<Component>())
			dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
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
		CopyComponent<LightComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<RigidBody3DComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<BoxCollider3DComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<NativeScriptComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<CameraComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<AudioComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<TerrainComponent>(dstRegistry, srcRegistry, enttMap);

		// Manually copy RelationshipComponent to remap entities
		auto relView = srcRegistry.view<RelationshipComponent>();
		for (auto e : relView) {
			entt::entity dstEnttID = enttMap.at(e);
			auto& srcRel = srcRegistry.get<RelationshipComponent>(e);
			auto& dstRel = dstRegistry.emplace_or_replace<RelationshipComponent>(dstEnttID);
			
			if (srcRel.Parent != entt::null && enttMap.find(srcRel.Parent) != enttMap.end()) {
				dstRel.Parent = enttMap.at(srcRel.Parent);
			}
			for (auto child : srcRel.Children) {
				if (enttMap.find(child) != enttMap.end()) {
					dstRel.Children.push_back(enttMap.at(child));
				}
			}
		}

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
		entity.AddComponent<RelationshipComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity) {
		// Clean up relationships
		if (m_Registry.any_of<RelationshipComponent>((entt::entity)entity)) {
			auto& rc = m_Registry.get<RelationshipComponent>((entt::entity)entity);
			// Copy children array since destroying children will modify it
			auto children = rc.Children;
			for (auto child : children) {
				DestroyEntity(Entity{ child, this });
			}

			// Remove from parent
			if (rc.Parent != entt::null) {
				auto& parentRc = m_Registry.get<RelationshipComponent>(rc.Parent);
				parentRc.Children.erase(std::remove(parentRc.Children.begin(), parentRc.Children.end(), (entt::entity)entity), parentRc.Children.end());
			}
		}

		// Clean up script instance if present
		if (m_Registry.any_of<NativeScriptComponent>((entt::entity)entity)) {
			auto& nsc = m_Registry.get<NativeScriptComponent>((entt::entity)entity);
			if (nsc.Instance) {
				nsc.Instance->OnDestroy();
				delete nsc.Instance;
				nsc.Instance = nullptr;
			}
		}

		// Clean up physics body
		b3BodyId destroyedBody = b3_nullBodyId;
		if (m_Registry.any_of<RigidBody3DComponent>((entt::entity)entity)) {
			auto& rb = m_Registry.get<RigidBody3DComponent>((entt::entity)entity);
			if (!B3_IS_NULL(rb.RuntimeBody)) {
				b3DestroyBody(rb.RuntimeBody);
				destroyedBody = rb.RuntimeBody;
			}
		}

        // Clean up terrain height field
        if (m_Registry.any_of<TerrainComponent>((entt::entity)entity)) {
            auto& terrain = m_Registry.get<TerrainComponent>((entt::entity)entity);
            if (terrain.RuntimeHeightField) {
                b3DestroyHeightField(terrain.RuntimeHeightField);
                terrain.RuntimeHeightField = nullptr;
            }
        }

		if (m_Registry.any_of<BoxCollider3DComponent>((entt::entity)entity)) {
			auto& bc = m_Registry.get<BoxCollider3DComponent>((entt::entity)entity);
			if (!B3_IS_NULL(bc.RuntimeBody) && !B3_ID_EQUALS(bc.RuntimeBody, destroyedBody)) {
				b3DestroyBody(bc.RuntimeBody);
			}
		}

		m_Registry.destroy(entity);
	}

	Entity Scene::DuplicateEntity(Entity entity) {
		std::string name = entity.GetComponent<TagComponent>().Tag;
		Entity newEntity = CreateEntity(name + " (Copy)");

		CopyComponentIfExists<TransformComponent>(newEntity, entity);
		CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
		CopyComponentIfExists<MeshComponent>(newEntity, entity);
		CopyComponentIfExists<LightComponent>(newEntity, entity);
		CopyComponentIfExists<RigidBody3DComponent>(newEntity, entity);
		CopyComponentIfExists<BoxCollider3DComponent>(newEntity, entity);
		CopyComponentIfExists<NativeScriptComponent>(newEntity, entity);
		CopyComponentIfExists<SkyboxComponent>(newEntity, entity);
		CopyComponentIfExists<CameraComponent>(newEntity, entity);
		CopyComponentIfExists<AudioComponent>(newEntity, entity);

		// Duplicate relationship (children recursively)
		if (entity.HasComponent<RelationshipComponent>()) {
			auto& srcRc = entity.GetComponent<RelationshipComponent>();
			auto& dstRc = newEntity.GetComponent<RelationshipComponent>();
			// We do NOT copy the parent! The duplicated entity is spawned at the root level by default.
			for (auto child : srcRc.Children) {
				Entity newChild = DuplicateEntity(Entity{ child, this });
				
				// Re-link parent-child for the duplicate
				auto& childRc = newChild.GetComponent<RelationshipComponent>();
				childRc.Parent = (entt::entity)newEntity;
				dstRc.Children.push_back((entt::entity)newChild);
			}
		}

		// Reset runtime physics bodies in the new entity so they are regenerated
		if (newEntity.HasComponent<RigidBody3DComponent>())
			newEntity.GetComponent<RigidBody3DComponent>().RuntimeBody = b3_nullBodyId;
		if (newEntity.HasComponent<BoxCollider3DComponent>())
			newEntity.GetComponent<BoxCollider3DComponent>().RuntimeBody = b3_nullBodyId;

		return newEntity;
	}

	Entity Scene::FindEntityByName(const std::string& name) {

		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view) {
			const auto& tc = view.get<TagComponent>(entity);
			if (tc.Tag == name)
				return Entity{ entity, this };
		}
		return {};
	}

	void Scene::OnUpdateEditor(float deltaTime) {
		// No physics step in editor mode
        
        // Dummy use to force template instantiation for HeronEditor to link against
        if (false) {
            auto view = m_Registry.view<TerrainComponent>();
            (void)view;
        }
	}

	void Scene::OnUpdateRuntime(float deltaTime) {
		// === Script Update ===
		{
			auto scriptView = m_Registry.view<NativeScriptComponent>();
			for (auto e : scriptView) {
				auto& nsc = scriptView.get<NativeScriptComponent>(e);

				if (!nsc.Instance && !nsc.ScriptName.empty()) {
					HRN_CORE_INFO("Instantiating script: {0}", nsc.ScriptName);
					nsc.Instance = ScriptRegistry::Create(nsc.ScriptName);
					if (nsc.Instance) {
						nsc.Instance->m_Entity = Entity{ e, this };
						nsc.Instance->OnCreate();
					}
				}

				if (nsc.Instance) {
					nsc.Instance->OnUpdate(deltaTime);
				}
			}
		}

		// === Physics Step ===
		{
			HRN_CORE_INFO("Physics Step START");
			static bool s_printedSize = false;
			if (!s_printedSize) {
				HRN_CORE_INFO("Scene.cpp sizeof(RigidBody3DComponent) = {0}", sizeof(RigidBody3DComponent));
				s_printedSize = true;
			}
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
					
					if (rb.FixedRotation) {
						bodyDef.motionLocks.angularX = true;
						bodyDef.motionLocks.angularY = true;
						bodyDef.motionLocks.angularZ = true;
					}

					rb.RuntimeBody = b3CreateBody(m_PhysicsWorld, &bodyDef);
					HRN_CORE_INFO("Physics Step: Created Body ID (index1={0}, world0={1}, gen={2})", rb.RuntimeBody.index1, rb.RuntimeBody.world0, rb.RuntimeBody.generation);

					// Initialize previous transform tracking
					rb.PreviousTranslation[0] = transform.Translation[0];
					rb.PreviousTranslation[1] = transform.Translation[1];
					rb.PreviousTranslation[2] = transform.Translation[2];
					rb.PreviousRotation[0] = transform.Rotation[0];
					rb.PreviousRotation[1] = transform.Rotation[1];
					rb.PreviousRotation[2] = transform.Rotation[2];

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

                    // Add Terrain HeightField if present
                    if (m_Registry.all_of<TerrainComponent>(e)) {
                        auto& terrain = m_Registry.get<TerrainComponent>(e);
                        if (!terrain.RuntimeHeightField && !terrain.RuntimeHeightData.empty()) {
                            
                            // 2. Define HeightField
                            b3HeightFieldDef hfDef = { 0 };
                            hfDef.heights = terrain.RuntimeHeightData.data();
                            hfDef.countX = terrain.HeightmapWidth;
                            hfDef.countZ = terrain.HeightmapHeight;
                            hfDef.globalMinimumHeight = 0.0f;
                            hfDef.globalMaximumHeight = terrain.HeightScale;
                            hfDef.scale = { terrain.Size / (float)(terrain.HeightmapWidth - 1), 
                                            1.0f, 
                                            terrain.Size / (float)(terrain.HeightmapHeight - 1) };
                            
                            terrain.RuntimeHeightField = b3CreateHeightField(&hfDef);
                            
                            b3ShapeDef shapeDef = b3DefaultShapeDef();
                            shapeDef.density = 0.0f; // Static terrain
                            b3CreateHeightFieldShape(rb.RuntimeBody, &shapeDef, terrain.RuntimeHeightField);
                        }
                    }
				} else {
					// Body already exists - check if script or gizmo modified the ECS transform
					bool positionChanged = false;
					b3Vec3 desiredVel = b3Body_GetLinearVelocity(rb.RuntimeBody);

					if (transform.Translation[0] != rb.PreviousTranslation[0]) {
					    desiredVel.x = (transform.Translation[0] - rb.PreviousTranslation[0]) / deltaTime;
					    positionChanged = true;
					}
					if (transform.Translation[1] != rb.PreviousTranslation[1]) {
					    desiredVel.y = (transform.Translation[1] - rb.PreviousTranslation[1]) / deltaTime;
					    positionChanged = true;
					}
					if (transform.Translation[2] != rb.PreviousTranslation[2]) {
					    desiredVel.z = (transform.Translation[2] - rb.PreviousTranslation[2]) / deltaTime;
					    positionChanged = true;
					}

					bool rotationChanged = false;
					if (transform.Rotation[0] != rb.PreviousRotation[0] ||
					    transform.Rotation[1] != rb.PreviousRotation[1] ||
					    transform.Rotation[2] != rb.PreviousRotation[2]) {
					    rotationChanged = true;
					}

					if (positionChanged) {
					    b3Body_SetLinearVelocity(rb.RuntimeBody, desiredVel);
					    b3Body_SetAwake(rb.RuntimeBody, true);
					}
					
					if (rotationChanged && !rb.FixedRotation) {
					    b3Quat rot = b3MakeQuatFromAxisAngle({1, 0, 0}, transform.Rotation[0]);
					    rot = b3MulQuat(rot, b3MakeQuatFromAxisAngle({0, 1, 0}, transform.Rotation[1]));
					    rot = b3MulQuat(rot, b3MakeQuatFromAxisAngle({0, 0, 1}, transform.Rotation[2]));
					    
					    b3Pos pos = b3Body_GetPosition(rb.RuntimeBody);
					    b3Body_SetTransform(rb.RuntimeBody, pos, rot);
					    b3Body_SetAwake(rb.RuntimeBody, true);
					}
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
			HRN_CORE_INFO("b3World_Step START");
			int subStepCount = 4;
			b3World_Step(m_PhysicsWorld, deltaTime, subStepCount);
			HRN_CORE_INFO("b3World_Step END");

			// Retrieve transforms from Physics engine
			for (auto e : view) {
				auto [rb, transform] = view.get<RigidBody3DComponent, TransformComponent>(e);
				if (!B3_IS_NULL(rb.RuntimeBody) && rb.Type == b3_dynamicBody) {
					b3Vec3 pos = b3Body_GetPosition(rb.RuntimeBody);
					transform.Translation[0] = pos.x;
					transform.Translation[1] = pos.y;
					transform.Translation[2] = pos.z;

					if (!rb.FixedRotation) {
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

					// Store these values so we can detect script/gizmo modifications next frame
					rb.PreviousTranslation[0] = transform.Translation[0];
					rb.PreviousTranslation[1] = transform.Translation[1];
					rb.PreviousTranslation[2] = transform.Translation[2];
					rb.PreviousRotation[0] = transform.Rotation[0];
					rb.PreviousRotation[1] = transform.Rotation[1];
					rb.PreviousRotation[2] = transform.Rotation[2];
				}
			}
			HRN_CORE_INFO("Physics Step END");
		}

		// === Audio Update ===
		{
			static std::mt19937 s_Rng(std::random_device{}());

			auto audioView = m_Registry.view<AudioComponent>();
			for (auto e : audioView) {
				auto& ac = audioView.get<AudioComponent>(e);
				if (ac.AudioPaths.empty()) continue;

				uint32_t entityId = (uint32_t)(entt::entity)e;

				if (ac.Play) {
					// First frame: start playing
					if (!ac.WasPlaying) {
						ac.CurrentTrackIndex = 0;
						if (ac.Shuffle) {
							std::uniform_int_distribution<int> dist(0, (int)ac.AudioPaths.size() - 1);
							ac.CurrentTrackIndex = dist(s_Rng);
						}
						AudioSystem::PlayTrack(entityId, ac.AudioPaths[ac.CurrentTrackIndex]);
						ac.WasPlaying = true;
					}
					// Track finished — advance to next
					else if (AudioSystem::IsTrackFinished(entityId)) {
						if (!ac.Loop && !ac.Shuffle) {
							// Sequential end: stop after last track
							if (ac.CurrentTrackIndex + 1 >= (int)ac.AudioPaths.size()) {
								ac.Play = false;
								ac.WasPlaying = false;
								continue;
							}
						}

						if (ac.Shuffle) {
							std::uniform_int_distribution<int> dist(0, (int)ac.AudioPaths.size() - 1);
							ac.CurrentTrackIndex = dist(s_Rng);
						} else {
							ac.CurrentTrackIndex = (ac.CurrentTrackIndex + 1) % (int)ac.AudioPaths.size();
						}
						AudioSystem::PlayTrack(entityId, ac.AudioPaths[ac.CurrentTrackIndex]);
					}
				} else {
					// Play was turned off
					if (ac.WasPlaying) {
						AudioSystem::StopTrack(entityId);
						ac.WasPlaying = false;
						ac.CurrentTrackIndex = -1;
					}
				}
			}
		}
	}

}
