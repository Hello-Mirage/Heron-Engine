#include "Heron/Heron.h"
#include <iostream>
#include <vector>
#include <random>

// Global game state struct (simple way to share state between scripts)
struct GameState {
    bool IsPlaying = true;
    int Score = 0;
};
static GameState s_GameState;

class PlayerController : public Heron::ScriptableEntity {
public:
    float ForwardSpeed = 10.0f;
    float SideSpeed = 8.0f;
    float MaxForwardSpeed = 40.0f;

    void OnCreate() override {
        HRN_CORE_INFO("PlayerController: Ready to run!");
        s_GameState.IsPlaying = true;
        s_GameState.Score = 0;
    }

    void OnUpdate(float ts) override {
        if (!s_GameState.IsPlaying) {
            return; // Dead
        }

        auto& transform = GetComponent<Heron::TransformComponent>();

        // Always move forward
        transform.Translation[2] -= ForwardSpeed * ts;

        // Accelerate over time
        ForwardSpeed += 0.5f * ts;
        if (ForwardSpeed > MaxForwardSpeed) ForwardSpeed = MaxForwardSpeed;

        // Dodging (X axis)
        if (Heron::Input::IsKeyPressed(Heron::Key::A)) {
            transform.Translation[0] -= SideSpeed * ts;
        }
        if (Heron::Input::IsKeyPressed(Heron::Key::D)) {
            transform.Translation[0] += SideSpeed * ts;
        }

        // Keep player within bounds (track is ~10 units wide)
        if (transform.Translation[0] < -5.0f) transform.Translation[0] = -5.0f;
        if (transform.Translation[0] > 5.0f) transform.Translation[0] = 5.0f;
    }
};

class CameraFollow : public Heron::ScriptableEntity {
public:
    Heron::Entity Player;
    float OffsetY = 3.0f;
    float OffsetZ = 6.0f;

    void OnCreate() override {
        Player = FindEntityByName("Player");
    }

    void OnUpdate(float ts) override {
        if (!Player) {
            Player = FindEntityByName("Player");
            if (!Player) return;
        }

        auto& myTransform = GetComponent<Heron::TransformComponent>();
        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();

        float targetX = playerTransform.Translation[0];
        
        myTransform.Translation[0] += (targetX - myTransform.Translation[0]) * 5.0f * ts;
        myTransform.Translation[1] = playerTransform.Translation[1] + OffsetY;
        myTransform.Translation[2] = playerTransform.Translation[2] + OffsetZ;
        
        myTransform.Rotation[0] = glm::radians(-15.0f);
    }
};

class ObstacleSpawner : public Heron::ScriptableEntity {
public:
    Heron::Entity Player;
    float SpawnDistanceAhead = 80.0f;
    float DespawnDistanceBehind = 10.0f;
    float SpawnIntervalZ = 15.0f; 
    float LastSpawnZ = 0.0f;

    struct ObstacleData {
        Heron::Entity entity;
        bool hasPassedPlayer;
    };
    std::vector<ObstacleData> ActiveObstacles;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> laneDist;

    void OnCreate() override {
        Player = FindEntityByName("Player");
        rng.seed(std::random_device()());
        laneDist = std::uniform_real_distribution<float>(-4.0f, 4.0f);
    }

    void OnUpdate(float ts) override {
        if (!s_GameState.IsPlaying) return;
        if (!Player) {
            Player = FindEntityByName("Player");
            if (!Player) return;
        }

        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();
        float currentZ = playerTransform.Translation[2];

        // Spawn new obstacles as the player moves forward
        if (currentZ < LastSpawnZ - SpawnIntervalZ) {
            SpawnObstacle(currentZ - SpawnDistanceAhead);
            LastSpawnZ = currentZ;
            
            SpawnIntervalZ -= 0.1f * ts;
            if (SpawnIntervalZ < 6.0f) SpawnIntervalZ = 6.0f;
        }

        // Check for passed obstacles & collisions
        float pX = playerTransform.Translation[0];
        float pZ = playerTransform.Translation[2];
        float pHalfSize = 0.5f;

        for (auto it = ActiveObstacles.begin(); it != ActiveObstacles.end(); ) {
            if (it->entity) {
                auto& obsTransform = it->entity.GetComponent<Heron::TransformComponent>();
                
                // Score counting: Check if obstacle was just crossed
                if (!it->hasPassedPlayer && obsTransform.Translation[2] > pZ) {
                    it->hasPassedPlayer = true;
                    s_GameState.Score++;
                    HRN_CORE_INFO("Score: {0}", s_GameState.Score);
                }

                // AABB Collision Detection
                float oX = obsTransform.Translation[0];
                float oZ = obsTransform.Translation[2];
                // Obstacle base half-sizes (scaled)
                float oHalfX = 0.5f * obsTransform.Scale[0];
                float oHalfZ = 0.5f * obsTransform.Scale[2];

                if (std::abs(pX - oX) < (pHalfSize + oHalfX) &&
                    std::abs(pZ - oZ) < (pHalfSize + oHalfZ)) {
                    // Collision!
                    s_GameState.IsPlaying = false;
                    HRN_CORE_WARN("GAME OVER! Final Score: {0}", s_GameState.Score);
                }

                // Despawn if far behind
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
    }
    
    void SpawnObstacle(float spawnZ) {
        Heron::Entity obstacle = Instantiate("Obstacle");
        auto& transform = obstacle.GetComponent<Heron::TransformComponent>();
        
        transform.Translation[0] = laneDist(rng);
        transform.Translation[1] = 0.5f; 
        transform.Translation[2] = spawnZ;
        
        transform.Scale[0] = 1.0f + (laneDist(rng) * 0.2f);
        transform.Scale[1] = 1.0f;
        transform.Scale[2] = 1.0f + (laneDist(rng) * 0.2f);

        auto& mesh = obstacle.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cube);
        mesh.Color[0] = 0.8f; mesh.Color[1] = 0.2f; mesh.Color[2] = 0.2f; mesh.Color[3] = 1.0f;

        ActiveObstacles.push_back({ obstacle, false });
    }
    
    void OnDestroy() override {
        for (auto obs : ActiveObstacles) {
            if (obs.entity) Destroy(obs.entity);
        }
        ActiveObstacles.clear();
    }
};

class GroundSpawner : public Heron::ScriptableEntity {
public:
    Heron::Entity Player;
    float SpawnDistanceAhead = 100.0f;
    float DespawnDistanceBehind = 20.0f;
    float GroundLength = 20.0f; 
    float LastSpawnZ = 0.0f;

    std::vector<Heron::Entity> ActiveGrounds;
    
    void OnCreate() override {
        Player = FindEntityByName("Player");
        // Spawn initial pieces
        for (int i = 0; i < 5; i++) {
            SpawnGround(-i * GroundLength);
            LastSpawnZ = -i * GroundLength;
        }
    }

    void OnUpdate(float ts) override {
        if (!Player) {
            Player = FindEntityByName("Player");
            if (!Player) return;
        }

        auto& playerTransform = Player.GetComponent<Heron::TransformComponent>();
        float currentZ = playerTransform.Translation[2];

        // Spawn new ground
        if (currentZ < LastSpawnZ - GroundLength / 2.0f + SpawnDistanceAhead) {
            SpawnGround(LastSpawnZ - GroundLength);
            LastSpawnZ -= GroundLength;
        }

        // Cleanup passed ground
        for (auto it = ActiveGrounds.begin(); it != ActiveGrounds.end(); ) {
            if (*it) {
                auto& gTransform = it->GetComponent<Heron::TransformComponent>();
                if (gTransform.Translation[2] > currentZ + DespawnDistanceBehind) {
                    Destroy(*it);
                    it = ActiveGrounds.erase(it);
                } else {
                    ++it;
                }
            } else {
                it = ActiveGrounds.erase(it);
            }
        }
    }
    
    void SpawnGround(float spawnZ) {
        Heron::Entity ground = Instantiate("Ground Segment");
        auto& transform = ground.GetComponent<Heron::TransformComponent>();
        
        transform.Translation[0] = 0.0f;
        transform.Translation[1] = -0.5f; // slightly below player
        transform.Translation[2] = spawnZ;
        
        transform.Scale[0] = 12.0f; // Width of the road
        transform.Scale[1] = 1.0f;
        transform.Scale[2] = GroundLength;

        auto& mesh = ground.AddComponent<Heron::MeshComponent>(Heron::PrimitiveType::Cube);
        // Checkerboard effect by alternating color based on spawnZ
        if (std::abs(fmod(spawnZ, GroundLength * 2.0f)) < 0.1f) {
            mesh.Color[0] = 0.1f; mesh.Color[1] = 0.1f; mesh.Color[2] = 0.15f; mesh.Color[3] = 1.0f;
        } else {
            mesh.Color[0] = 0.15f; mesh.Color[1] = 0.15f; mesh.Color[2] = 0.2f; mesh.Color[3] = 1.0f;
        }

        ActiveGrounds.push_back(ground);
    }
    
    void OnDestroy() override {
        for (auto g : ActiveGrounds) {
            if (g) Destroy(g);
        }
        ActiveGrounds.clear();
    }
};

HERON_REGISTER_SCRIPT(PlayerController);
HERON_REGISTER_SCRIPT(CameraFollow);
HERON_REGISTER_SCRIPT(ObstacleSpawner);
HERON_REGISTER_SCRIPT(GroundSpawner);

extern "C" __declspec(dllexport) void InitializeGame() {
    std::cout << "GameProject DLL Initialized (Endless Runner)." << std::endl;
}
