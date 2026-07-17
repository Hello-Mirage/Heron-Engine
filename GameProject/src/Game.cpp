#include "Heron/Heron.h"
#include <iostream>
#include <vector>
#include <random>

class PlayerController : public Heron::ScriptableEntity {
public:
    float ForwardSpeed = 10.0f;
    float SideSpeed = 8.0f;
    float MaxForwardSpeed = 30.0f;
    float MinForwardSpeed = 5.0f;

    void OnCreate() override {
        std::cout << "PlayerController: Ready to run!" << std::endl;
    }

    void OnUpdate(float ts) override {
        auto& transform = GetComponent<Heron::TransformComponent>();

        // Always move forward (negative Z is forward in our coordinate system usually)
        transform.Translation[2] -= ForwardSpeed * ts;

        // Speed control
        if (Heron::Input::IsKeyPressed(Heron::Key::W)) {
            ForwardSpeed += 5.0f * ts;
            if (ForwardSpeed > MaxForwardSpeed) ForwardSpeed = MaxForwardSpeed;
        } else if (Heron::Input::IsKeyPressed(Heron::Key::S) || Heron::Input::IsKeyPressed(Heron::Key::Space)) {
            ForwardSpeed -= 15.0f * ts;
            if (ForwardSpeed < MinForwardSpeed) ForwardSpeed = MinForwardSpeed;
        }

        // Dodging (X axis)
        if (Heron::Input::IsKeyPressed(Heron::Key::A)) {
            transform.Translation[0] -= SideSpeed * ts;
        }
        if (Heron::Input::IsKeyPressed(Heron::Key::D)) {
            transform.Translation[0] += SideSpeed * ts;
        }

        // Keep player within bounds (optional, assuming track is ~10 units wide)
        if (transform.Translation[0] < -5.0f) transform.Translation[0] = -5.0f;
        if (transform.Translation[0] > 5.0f) transform.Translation[0] = 5.0f;
    }
};

class CameraFollow : public Heron::ScriptableEntity {
public:
    Heron::Entity Player;
    float OffsetY = 3.0f;
    float OffsetZ = 6.0f; // Positive Z means behind the player (if player moves -Z)

    void OnCreate() override {
        Player = FindEntityByName("Player");
        if (!Player) {
            std::cout << "CameraFollow: Could not find entity named 'Player'!" << std::endl;
        }
    }

    void OnUpdate(float ts) override {
        if (!Player) {
            // Try finding it again if it was created after the camera
            Player = FindEntityByName("Player");
            if (!Player) return;
        }

        auto& myTransform = GetComponent<Heron::TransformComponent>();
        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();

        // Follow behind and slightly above
        // We smooth the X movement slightly, but lock the Z movement
        float targetX = playerTransform.Translation[0];
        
        // Simple lerp for X to make the camera feel a bit dynamic
        myTransform.Translation[0] += (targetX - myTransform.Translation[0]) * 5.0f * ts;
        
        myTransform.Translation[1] = playerTransform.Translation[1] + OffsetY;
        myTransform.Translation[2] = playerTransform.Translation[2] + OffsetZ;
        
        // Slight downward tilt (pitch)
        myTransform.Rotation[0] = glm::radians(-15.0f);
    }
};

class ObstacleSpawner : public Heron::ScriptableEntity {
public:
    Heron::Entity Player;
    float SpawnDistanceAhead = 60.0f;
    float DespawnDistanceBehind = 10.0f;
    float SpawnIntervalZ = 15.0f; 
    float LastSpawnZ = 0.0f;

    std::vector<Heron::Entity> ActiveObstacles;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> laneDist;

    void OnCreate() override {
        Player = FindEntityByName("Player");
        rng.seed(std::random_device()());
        laneDist = std::uniform_real_distribution<float>(-4.0f, 4.0f);
    }

    void OnUpdate(float ts) override {
        if (!Player) {
            Player = FindEntityByName("Player");
            if (!Player) return;
        }

        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();
        float currentZ = playerTransform.Translation[2];

        // Spawn new obstacles as the player moves forward (negative Z)
        if (currentZ < LastSpawnZ - SpawnIntervalZ) {
            SpawnObstacle(currentZ - SpawnDistanceAhead);
            LastSpawnZ = currentZ;
            
            // Slightly increase difficulty over time
            SpawnIntervalZ -= 0.1f * ts;
            if (SpawnIntervalZ < 5.0f) SpawnIntervalZ = 5.0f;
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
    }
    
    void SpawnObstacle(float spawnZ) {
        Heron::Entity obstacle = Instantiate("Obstacle");
        auto& transform = obstacle.GetComponent<Heron::TransformComponent>();
        
        // Random lane placement
        transform.Translation[0] = laneDist(rng);
        // Floor height
        transform.Translation[1] = 0.5f; 
        transform.Translation[2] = spawnZ;
        
        // Random scale variation
        transform.Scale[0] = 1.0f + (laneDist(rng) * 0.2f);
        transform.Scale[1] = 1.0f;
        transform.Scale[2] = 1.0f + (laneDist(rng) * 0.2f);

        auto& mesh = obstacle.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cube);
        // Make it red
        mesh.Color[0] = 0.8f; mesh.Color[1] = 0.2f; mesh.Color[2] = 0.2f; mesh.Color[3] = 1.0f;

        // Add physics body if we want them to physically collide (static body)
        // auto& rb = obstacle.AddComponent<Heron::RigidBody3DComponent>();
        // rb.Type = b3_staticBody;
        // auto& bc = obstacle.AddComponent<Heron::BoxCollider3DComponent>();
        
        ActiveObstacles.push_back(obstacle);
    }
    
    void OnDestroy() override {
        // Cleanup remaining obstacles if spawner is destroyed
        for (auto obs : ActiveObstacles) {
            if (obs) Destroy(obs);
        }
        ActiveObstacles.clear();
    }
};

HERON_REGISTER_SCRIPT(PlayerController);
HERON_REGISTER_SCRIPT(CameraFollow);
HERON_REGISTER_SCRIPT(ObstacleSpawner);

extern "C" __declspec(dllexport) void InitializeGame() {
    std::cout << "GameProject DLL Initialized (Endless Runner)." << std::endl;
}
