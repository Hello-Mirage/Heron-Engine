#include <QApplication>
#include "EditorWindow.h"
#include <Heron/Core/Log.h>
#include <Heron/Core/Input.h>
#include <QLabel>

extern QLabel* g_ScoreLabel;
#include <Heron/Core/KeyCodes.h>
#include <Heron/Scene/Components.h>
#include <Heron/Scripting/ScriptableEntity.h>
#include <Heron/Scripting/ScriptRegistry.h>
#include <Heron/Audio/AudioSystem.h>
#include <random>
#include <vector>
#include <Heron/Renderer/TerrainGenerator.h>// ============================================================
//  BUILT-IN SCRIPTS — Available in the Editor's Script dropdown
// ============================================================

#include <Heron/Core/MouseCodes.h>
#include <glm/glm.hpp>

class FirstPersonController : public Heron::ScriptableEntity
{
public:
    float WalkSpeed = 8.0f;
    float SprintSpeed = 16.0f;
    float LookSensitivity = 1.0f;
    float JumpForce = 5.0f;
    float EyeHeight = 0.8f; // Height of the camera relative to the player center
    
    Heron::Entity CameraEntity;
    
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    bool m_IsLocked = false;

    void OnCreate() override {
        HRN_CORE_INFO("FirstPersonController: Initializing...");
        // Find child camera
        if (HasComponent<Heron::RelationshipComponent>()) {
            auto& rc = GetComponent<Heron::RelationshipComponent>();
            for (auto childId : rc.Children) {
                Heron::Entity child = { childId, GetScene() };
                if (child.HasComponent<Heron::CameraComponent>()) {
                    CameraEntity = child;
                    HRN_CORE_INFO("FirstPersonController: Found child camera!");
                    
                    // Snap the camera to the "head" position so it pivots nicely without revolving
                    auto& camTransform = CameraEntity.GetComponent<Heron::TransformComponent>();
                    auto& myTransform = GetComponent<Heron::TransformComponent>();
                    camTransform.Translation[0] = myTransform.Translation[0];
                    camTransform.Translation[1] = myTransform.Translation[1] + EyeHeight;
                    camTransform.Translation[2] = myTransform.Translation[2];
                    break;
                }
            }
        }
        
        if (!CameraEntity) {
            HRN_CORE_WARN("FirstPersonController needs a Camera component on a child entity!");
        }
    }

    void OnUpdate(float dt) override {
        // Toggle Lock
        if (Heron::Input::IsMouseButtonPressed(Heron::Mouse::ButtonLeft)) {
            if (!m_IsLocked && Heron::Input::IsViewportHovered()) {
                Heron::Input::SetCursorMode(Heron::CursorMode::Locked);
                m_IsLocked = true;
                Heron::Input::SetMousePosition(500.0f, 500.0f);
            }
        }
        if (Heron::Input::IsKeyPressed(Heron::Key::Escape)) {
            if (m_IsLocked) {
                Heron::Input::SetCursorMode(Heron::CursorMode::Normal);
                m_IsLocked = false;
            }
        }

        // 1. Mouse Look
        if (m_IsLocked) {
            float mouseX = Heron::Input::GetMouseX();
            float mouseY = Heron::Input::GetMouseY();
            
            float xOffset = mouseX - 500.0f;
            float yOffset = 500.0f - mouseY; 
            
            Heron::Input::SetMousePosition(500.0f, 500.0f);
            
            xOffset *= LookSensitivity * dt * 0.2f;
            yOffset *= LookSensitivity * dt * 0.2f;
            
            m_Yaw -= xOffset;
            m_Pitch += yOffset;
            
            if (m_Pitch > 1.5f) m_Pitch = 1.5f; 
            if (m_Pitch < -1.5f) m_Pitch = -1.5f;
            
            // Apply Yaw to player
            auto& transform = GetComponent<Heron::TransformComponent>();
            transform.Rotation[1] = m_Yaw;
            
            // Apply Pitch to camera
            if (CameraEntity) {
                auto& camTransform = CameraEntity.GetComponent<Heron::TransformComponent>();
                camTransform.Rotation[0] = m_Pitch;
                
                // Track the player's position (plus EyeHeight offset) since transforms are global
                camTransform.Translation[0] = transform.Translation[0];
                camTransform.Translation[1] = transform.Translation[1] + EyeHeight;
                camTransform.Translation[2] = transform.Translation[2];
            }
        }
        
        // 2. Keyboard Movement
        auto& transform = GetComponent<Heron::TransformComponent>();
        float speed = Heron::Input::IsKeyPressed(Heron::Key::LeftShift) ? SprintSpeed : WalkSpeed;
        
        glm::vec3 forward(sin(m_Yaw), 0.0f, cos(m_Yaw));
        glm::vec3 right(cos(m_Yaw), 0.0f, -sin(m_Yaw));
        
        glm::vec3 velocity(0.0f);
        
        if (Heron::Input::IsKeyPressed(Heron::Key::W)) velocity -= forward;
        if (Heron::Input::IsKeyPressed(Heron::Key::S)) velocity += forward;
        if (Heron::Input::IsKeyPressed(Heron::Key::A)) velocity -= right;
        if (Heron::Input::IsKeyPressed(Heron::Key::D)) velocity += right;
        
        if (glm::length(velocity) > 0.0f) {
            velocity = glm::normalize(velocity) * speed;
        }
        
        // 3. Apply Movement
        if (HasComponent<Heron::RigidBody3DComponent>()) {
            auto& rb = GetComponent<Heron::RigidBody3DComponent>();
            if (!B3_IS_NULL(rb.RuntimeBody)) {
                b3Vec3 currentVel = b3Body_GetLinearVelocity(rb.RuntimeBody);
                
                b3Vec3 targetVel;
                targetVel.x = velocity.x;
                targetVel.y = currentVel.y;
                targetVel.z = velocity.z;
                
                if (Heron::Input::IsKeyPressed(Heron::Key::Space) && currentVel.y < 0.1f && currentVel.y > -0.1f) {
                    targetVel.y = JumpForce;
                }
                
                b3Body_SetLinearVelocity(rb.RuntimeBody, targetVel);
                
                b3Quat rot = b3MakeQuatFromAxisAngle({0, 1, 0}, m_Yaw);
                b3Pos pos = b3Body_GetPosition(rb.RuntimeBody);
                b3Body_SetTransform(rb.RuntimeBody, pos, rot);
            }
        } else {
            transform.Translation[0] += velocity.x * dt;
            transform.Translation[2] += velocity.z * dt;
        }
    }
};
HERON_REGISTER_SCRIPT(FirstPersonController);

class PlayerController : public Heron::ScriptableEntity
{
public:
    float ForwardSpeed = 10.0f;
    float SideSpeed = 8.0f;
    float MaxForwardSpeed = 30.0f;
    float MinForwardSpeed = 5.0f;
    float Score = 0.0f;

    void OnCreate() override {
        HRN_CORE_INFO("PlayerController: Ready to run!");
        Score = 0.0f;
    }

    void OnUpdate(float dt) override {
        HRN_CORE_INFO("PlayerController::OnUpdate START");
        auto& transform = GetComponent<Heron::TransformComponent>();

        // Game Over Check: If we fell off the edge or bounced into the air
        if (transform.Translation[1] < -2.0f || transform.Translation[1] > 5.0f) {
            HRN_CORE_WARN("CRASH! Final Score: {0}", (int)Score);
            ResetGame();
            return;
        }

        if (g_ScoreLabel) {
            g_ScoreLabel->setText(QString("Score: %1").arg((int)Score));
        }

        // Speed control
        if (Heron::Input::IsKeyPressed(Heron::Key::W)) {
            ForwardSpeed += 5.0f * dt;
            if (ForwardSpeed > MaxForwardSpeed) ForwardSpeed = MaxForwardSpeed;
        } else if (Heron::Input::IsKeyPressed(Heron::Key::S) || Heron::Input::IsKeyPressed(Heron::Key::Space)) {
            ForwardSpeed -= 15.0f * dt;
            if (ForwardSpeed < MinForwardSpeed) ForwardSpeed = MinForwardSpeed;
        }

        float currentSideSpeed = 0.0f;
        if (Heron::Input::IsKeyPressed(Heron::Key::A)) currentSideSpeed = -SideSpeed;
        if (Heron::Input::IsKeyPressed(Heron::Key::D)) currentSideSpeed = SideSpeed;

        // Use Physics if available for smooth movement
        if (HasComponent<Heron::RigidBody3DComponent>()) {
            static bool s_printedSize = false;
            if (!s_printedSize) {
                HRN_CORE_INFO("main.cpp sizeof(RigidBody3DComponent) = {0}", sizeof(Heron::RigidBody3DComponent));
                s_printedSize = true;
            }
            HRN_CORE_INFO("PlayerController: Has RigidBody3DComponent");
            auto& rb = GetComponent<Heron::RigidBody3DComponent>();
            if (!B3_IS_NULL(rb.RuntimeBody)) {
                HRN_CORE_INFO("PlayerController: b3Body_GetLinearVelocity START. Body ID (index1={0}, world0={1}, gen={2})", rb.RuntimeBody.index1, rb.RuntimeBody.world0, rb.RuntimeBody.generation);
                b3Vec3 vel = b3Body_GetLinearVelocity(rb.RuntimeBody);
                HRN_CORE_INFO("PlayerController: b3Body_GetLinearVelocity END");
                vel.z = -ForwardSpeed;
                
                // Allow physics to bounce us, but override X and Z
                if (currentSideSpeed != 0.0f) {
                    vel.x = currentSideSpeed;
                } else {
                    vel.x = 0.0f; // Stop sliding when key released
                }
                
                // Keep within bounds
                HRN_CORE_INFO("PlayerController: b3Body_GetPosition START");
                b3Pos pos = b3Body_GetPosition(rb.RuntimeBody);
                HRN_CORE_INFO("PlayerController: b3Body_GetPosition END");
                if (pos.x < -5.0f && vel.x < 0) vel.x = 0;
                if (pos.x > 5.0f && vel.x > 0) vel.x = 0;

                HRN_CORE_INFO("PlayerController: b3Body_SetLinearVelocity START");
                b3Body_SetLinearVelocity(rb.RuntimeBody, vel);
                HRN_CORE_INFO("PlayerController: b3Body_SetLinearVelocity END");
            }
        } else {
            // Fallback to transform translation
            transform.Translation[2] -= ForwardSpeed * dt;
            transform.Translation[0] += currentSideSpeed * dt;
            
            // Keep within bounds
            if (transform.Translation[0] < -5.0f) transform.Translation[0] = -5.0f;
            if (transform.Translation[0] > 5.0f) transform.Translation[0] = 5.0f;
        }
        HRN_CORE_INFO("PlayerController::OnUpdate END");
    }

    void ResetGame() {
        auto& transform = GetComponent<Heron::TransformComponent>();
        transform.Translation[0] = 0.0f;
        transform.Translation[1] = 0.5f; // Back on the ground
        transform.Translation[2] = 0.0f;
        transform.Rotation[0] = 0.0f;
        transform.Rotation[1] = 0.0f;
        transform.Rotation[2] = 0.0f;
        ForwardSpeed = 10.0f;
        Score = 0.0f;
        
        // Reset rigid body velocity if it has one
        if (HasComponent<Heron::RigidBody3DComponent>()) {
            auto& rb = GetComponent<Heron::RigidBody3DComponent>();
            if (!B3_IS_NULL(rb.RuntimeBody)) {
                b3Body_SetLinearVelocity(rb.RuntimeBody, {0,0,0});
                b3Body_SetAngularVelocity(rb.RuntimeBody, {0,0,0});
            }
        }
    }

    void OnDestroy() override {
        HRN_CORE_INFO("PlayerController: OnDestroy");
    }
};
HERON_REGISTER_SCRIPT(PlayerController);

class CameraFollow : public Heron::ScriptableEntity
{
public:
    Heron::Entity Player;
    float OffsetY = 3.0f;
    float OffsetZ = 6.0f;

    void OnCreate() override {
        Player = FindEntityByName("Player");
        if (!Player)
            HRN_CORE_WARN("CameraFollow: Could not find entity named 'Player'!");
    }

    void OnUpdate(float dt) override {
        if (!Player) {
            Player = FindEntityByName("Player");
            if (!Player) return;
        }

        auto& myTransform = GetComponent<Heron::TransformComponent>();
        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();

        // Smoothly interpolate ALL axes to absorb physics micro-jitters
        float targetX = playerTransform.Translation[0];
        float targetY = playerTransform.Translation[1] + OffsetY;
        float targetZ = playerTransform.Translation[2] + OffsetZ;
        
        myTransform.Translation[0] += (targetX - myTransform.Translation[0]) * 15.0f * dt;
        myTransform.Translation[1] += (targetY - myTransform.Translation[1]) * 15.0f * dt;
        myTransform.Translation[2] += (targetZ - myTransform.Translation[2]) * 20.0f * dt;

        // Slight downward tilt
        myTransform.Rotation[0] = -0.26f; // ~15 degrees down
    }
};
HERON_REGISTER_SCRIPT(CameraFollow);

class ObstacleSpawner : public Heron::ScriptableEntity
{
public:
    Heron::Entity Player;
    float SpawnDistanceAhead = 60.0f;
    float DespawnDistanceBehind = 10.0f;
    float SpawnIntervalZ = 15.0f;
    float LastSpawnZ = 0.0f;

    struct ObstacleData {
        Heron::Entity entity;
        bool hasPassedPlayer;
    };
    std::vector<ObstacleData> ActiveObstacles;
    std::vector<Heron::Entity> ActiveDebris;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> laneDist;

    void OnCreate() override {
        Player = FindEntityByName("Player");
        rng.seed(std::random_device()());
        laneDist = std::uniform_real_distribution<float>(-4.0f, 4.0f);
    }

    void OnUpdate(float dt) override {
        if (!Player) {
            Player = FindEntityByName("Player");
            if (!Player) return;
        }

        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();
        float currentZ = playerTransform.Translation[2];
        
        // If player reset to start, clear obstacles
        if (currentZ > -5.0f && LastSpawnZ < -20.0f) {
            for (auto obs : ActiveObstacles) {
                if (obs.entity) Destroy(obs.entity);
            }
            ActiveObstacles.clear();

            for (auto debris : ActiveDebris) {
                if (debris) Destroy(debris);
            }
            ActiveDebris.clear();

            LastSpawnZ = 0.0f;

            SpawnIntervalZ = 15.0f;
        }

        // Spawn obstacles as player advances
        if (currentZ < LastSpawnZ - SpawnIntervalZ) {
            SpawnObstacle(currentZ - SpawnDistanceAhead);
            LastSpawnZ = currentZ;
            
            SpawnIntervalZ -= 0.1f * dt;
            if (SpawnIntervalZ < 4.0f) SpawnIntervalZ = 4.0f;
            
            std::uniform_real_distribution<float> jitterDist(-1.5f, 1.5f);
            LastSpawnZ += jitterDist(rng);
        }

        float pX = playerTransform.Translation[0];
        float pZ = playerTransform.Translation[2];
        float pY = playerTransform.Translation[1];
        float pHalf = 0.5f;
        float oHalf = 0.5f;

        // Cleanup passed obstacles & check collision
        for (auto it = ActiveObstacles.begin(); it != ActiveObstacles.end(); ) {
            if (it->entity) {
                auto& obsTransform = it->entity.GetComponent<Heron::TransformComponent>();
                
                float oX = obsTransform.Translation[0];
                float oZ = obsTransform.Translation[2];
                float oY = obsTransform.Translation[1];
                
                // AABB Collision (smash the block)
                if (std::abs(pX - oX) < (pHalf + oHalf) && std::abs(pZ - oZ) < (pHalf + oHalf) && std::abs(pY - oY) < (pHalf + oHalf)) {
                    if (Player.HasComponent<Heron::NativeScriptComponent>()) {
                        auto* nsc = &Player.GetComponent<Heron::NativeScriptComponent>();
                        if (nsc->Instance) {
                            PlayerController* pc = (PlayerController*)nsc->Instance;
                            pc->Score += 1.0f; // Scored a point!
                        }
                    }
                    SpawnDebris(oX, oY, oZ);
                    Destroy(it->entity);
                    it = ActiveObstacles.erase(it);
                    continue;
                }
                
                // Passed player without collision
                if (!it->hasPassedPlayer && obsTransform.Translation[2] > currentZ + oHalf) {
                    it->hasPassedPlayer = true;
                    if (Player.HasComponent<Heron::NativeScriptComponent>()) {
                        auto* nsc = &Player.GetComponent<Heron::NativeScriptComponent>();
                        if (nsc->Instance) {
                            PlayerController* pc = (PlayerController*)nsc->Instance;
                            pc->Score -= 1.0f; // Penalize for missing
                        }
                    }
                }
                
                if (obsTransform.Translation[2] > currentZ + DespawnDistanceBehind) {
                    Destroy(it->entity);
                    it = ActiveObstacles.erase(it);
                } else {
                    ++it;
                }
            } else {
                it = ActiveObstacles.erase(it);
            }
        }

        // Cleanup Debris
        for (auto it = ActiveDebris.begin(); it != ActiveDebris.end(); ) {
            if (*it) {
                auto& debrisTransform = it->GetComponent<Heron::TransformComponent>();
                // Despawn if it falls below kill plane (-10 Y) or falls too far behind player
                if (debrisTransform.Translation[1] < -10.0f || debrisTransform.Translation[2] > currentZ + DespawnDistanceBehind) {
                    Destroy(*it);
                    it = ActiveDebris.erase(it);
                } else {
                    ++it;
                }
            } else {
                it = ActiveDebris.erase(it);
            }
        }
    }

    void SpawnDebris(float oX, float oY, float oZ) {

        for (int i = 0; i < 5; i++) {
            Heron::Entity debris = Instantiate("Debris");
            auto& transform = debris.GetComponent<Heron::TransformComponent>();
            transform.Translation[0] = oX + (laneDist(rng) * 0.1f);
            transform.Translation[1] = oY + (laneDist(rng) * 0.1f);
            transform.Translation[2] = oZ + (laneDist(rng) * 0.1f);
            transform.Scale[0] = 0.2f;
            transform.Scale[1] = 0.2f;
            transform.Scale[2] = 0.2f;

            auto& mesh = debris.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cube);
            mesh.Color[0] = 0.8f; mesh.Color[1] = 0.2f; mesh.Color[2] = 0.2f; mesh.Color[3] = 1.0f;
            
            auto& rb = debris.AddComponent<Heron::RigidBody3DComponent>();
            rb.Type = b3_dynamicBody; // Allows physics to scatter it
            auto& bc = debris.AddComponent<Heron::BoxCollider3DComponent>();
            bc.HalfSize[0] = 0.1f; bc.HalfSize[1] = 0.1f; bc.HalfSize[2] = 0.1f;
            
            ActiveDebris.push_back(debris);
        }
    }

    void SpawnObstacle(float spawnZ) {

        Heron::Entity obstacle = Instantiate("Obstacle");
        auto& transform = obstacle.GetComponent<Heron::TransformComponent>();

        transform.Translation[0] = laneDist(rng);
        transform.Translation[1] = 0.5f;
        transform.Translation[2] = spawnZ;

        auto& mesh = obstacle.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cube);
        mesh.Color[0] = 0.8f; mesh.Color[1] = 0.2f; mesh.Color[2] = 0.2f; mesh.Color[3] = 1.0f;
        
        // NO physics components so the player can pass through and "cut" it
        ActiveObstacles.push_back({ obstacle, false });
    }

    void OnDestroy() override {
        for (auto obs : ActiveObstacles) {
            if (obs.entity) Destroy(obs.entity);
        }
        ActiveObstacles.clear();

        for (auto debris : ActiveDebris) {
            if (debris) Destroy(debris);
        }
        ActiveDebris.clear();
    }
};
HERON_REGISTER_SCRIPT(ObstacleSpawner);


class RotateScript : public Heron::ScriptableEntity
{
public:
    void OnUpdate(float dt) override {
        auto& transform = GetComponent<Heron::TransformComponent>();
        transform.Rotation[1] += 2.0f * dt; // spin around Y axis
    }
};
HERON_REGISTER_SCRIPT(RotateScript);

int main(int argc, char *argv[])
{
    freopen("stdout.log", "w", stdout);
    freopen("stderr.log", "w", stderr);
    Heron::Log::Init();
    QApplication a(argc, argv);
    
    // Set a dark fusion theme
    a.setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    Heron::AudioSystem::Init();
    EditorWindow w;
    w.show();
    int ret = a.exec();
    Heron::AudioSystem::Shutdown();
    return ret;
}
