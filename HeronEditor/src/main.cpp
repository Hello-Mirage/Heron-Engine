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
#include <random>
#include <vector>

// ============================================================
//  BUILT-IN SCRIPTS — Available in the Editor's Script dropdown
// ============================================================

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

        Score += ForwardSpeed * dt * 0.1f; // Increase score based on distance
        
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
        HRN_CORE_INFO("CameraFollow::OnUpdate START");
        if (!Player) {
            Player = FindEntityByName("Player");
            if (!Player) {
                HRN_CORE_INFO("CameraFollow::OnUpdate END (No Player)");
                return;
            }
        }

        auto& myTransform = GetComponent<Heron::TransformComponent>();
        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();

        // Smoothly interpolate ALL axes to absorb physics micro-jitters
        float targetX = playerTransform.Translation[0];
        float targetY = playerTransform.Translation[1] + OffsetY;
        float targetZ = playerTransform.Translation[2] + OffsetZ;
        
        myTransform.Translation[0] += (targetX - myTransform.Translation[0]) * 5.0f * dt;
        myTransform.Translation[1] += (targetY - myTransform.Translation[1]) * 10.0f * dt;
        myTransform.Translation[2] += (targetZ - myTransform.Translation[2]) * 15.0f * dt;

        // Slight downward tilt
        myTransform.Rotation[0] = -0.26f; // ~15 degrees down
        HRN_CORE_INFO("CameraFollow::OnUpdate END");
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
    float LastLineSpawnZ = 0.0f;
    float LineSpawnIntervalZ = 10.0f;

    std::vector<Heron::Entity> ActiveObstacles;
    std::vector<Heron::Entity> ActiveLines;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> laneDist;

    void OnCreate() override {
        Player = FindEntityByName("Player");
        rng.seed(std::random_device()());
        laneDist = std::uniform_real_distribution<float>(-4.0f, 4.0f);
    }

    void OnUpdate(float dt) override {
        HRN_CORE_INFO("ObstacleSpawner::OnUpdate START");
        if (!Player) {
            Player = FindEntityByName("Player");
            if (!Player) {
                HRN_CORE_INFO("ObstacleSpawner::OnUpdate END (No Player)");
                return;
            }
        }

        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();
        float currentZ = playerTransform.Translation[2];
        
        // If player reset to start, clear obstacles and lines
        if (currentZ > -5.0f && LastSpawnZ < -20.0f) {
            for (auto obs : ActiveObstacles) {
                if (obs) Destroy(obs);
            }
            ActiveObstacles.clear();
            
            for (auto line : ActiveLines) {
                if (line) Destroy(line);
            }
            ActiveLines.clear();
            
            LastSpawnZ = 0.0f;
            LastLineSpawnZ = 0.0f;
            SpawnIntervalZ = 15.0f;
        }

        // Spawn obstacles as player advances
        if (currentZ < LastSpawnZ - SpawnIntervalZ) {
            SpawnObstacle(currentZ - SpawnDistanceAhead);
            LastSpawnZ = currentZ;
            
            // Speed up spawning to increase difficulty
            SpawnIntervalZ -= 0.1f * dt;
            if (SpawnIntervalZ < 4.0f) SpawnIntervalZ = 4.0f;
        }
        
        // Spawn road lines
        if (currentZ < LastLineSpawnZ - LineSpawnIntervalZ) {
            SpawnRoadLine(currentZ - SpawnDistanceAhead);
            LastLineSpawnZ = currentZ;
        }

        // Cleanup passed obstacles
        for (auto it = ActiveObstacles.begin(); it != ActiveObstacles.end(); ) {
            if (*it) {
                auto& obsTransform = it->GetComponent<Heron::TransformComponent>();
                if (obsTransform.Translation[2] > currentZ + DespawnDistanceBehind) {
                    Destroy(*it);
                    it = ActiveObstacles.erase(it);
                } else {
                    ++it;
                }
            } else {
                it = ActiveObstacles.erase(it);
            }
        }
        
        // Cleanup passed road lines
        for (auto it = ActiveLines.begin(); it != ActiveLines.end(); ) {
            if (*it) {
                auto& lineTransform = it->GetComponent<Heron::TransformComponent>();
                if (lineTransform.Translation[2] > currentZ + DespawnDistanceBehind) {
                    Destroy(*it);
                    it = ActiveLines.erase(it);
                } else {
                    ++it;
                }
            } else {
                it = ActiveLines.erase(it);
            }
        }
        HRN_CORE_INFO("ObstacleSpawner::OnUpdate END");
    }

    void SpawnObstacle(float spawnZ) {
        Heron::Entity obstacle = Instantiate("Obstacle");
        auto& transform = obstacle.GetComponent<Heron::TransformComponent>();

        transform.Translation[0] = laneDist(rng);
        transform.Translation[1] = 0.5f;
        transform.Translation[2] = spawnZ;

        auto& mesh = obstacle.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cube);
        mesh.Color[0] = 0.8f; mesh.Color[1] = 0.2f; mesh.Color[2] = 0.2f; mesh.Color[3] = 1.0f;
        
        // Add Physics!
        auto& rb = obstacle.AddComponent<Heron::RigidBody3DComponent>();
        rb.Type = b3_staticBody; // Doesn't move
        auto& bc = obstacle.AddComponent<Heron::BoxCollider3DComponent>();
        bc.HalfSize[0] = 0.5f; bc.HalfSize[1] = 0.5f; bc.HalfSize[2] = 0.5f;

        ActiveObstacles.push_back(obstacle);
    }
    
    void SpawnRoadLine(float spawnZ) {
        Heron::Entity line = Instantiate("RoadLine");
        auto& transform = line.GetComponent<Heron::TransformComponent>();

        transform.Translation[0] = 0.0f;
        transform.Translation[1] = -0.49f; // Just above the ground floor
        transform.Translation[2] = spawnZ;
        
        // Long, flat, narrow rectangle
        transform.Scale[0] = 0.2f;
        transform.Scale[1] = 0.02f;
        transform.Scale[2] = 4.0f;

        auto& mesh = line.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cube);
        mesh.Color[0] = 1.0f; mesh.Color[1] = 1.0f; mesh.Color[2] = 1.0f; mesh.Color[3] = 1.0f;

        // No Physics! Just visual
        ActiveLines.push_back(line);
    }

    void OnDestroy() override {
        for (auto obs : ActiveObstacles) {
            if (obs) Destroy(obs);
        }
        ActiveObstacles.clear();
        
        for (auto line : ActiveLines) {
            if (line) Destroy(line);
        }
        ActiveLines.clear();
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
    EditorWindow w;
    w.show();
    return a.exec();
}
