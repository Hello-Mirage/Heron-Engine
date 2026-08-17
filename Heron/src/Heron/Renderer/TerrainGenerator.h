#pragma once

#include "Heron/Renderer/VertexArray.h"
#include "Heron/Scene/Components.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace Heron {

    class TerrainGenerator {
    public:
        // Terrain Mesh Generation
        static Heron::Ref<Heron::VertexArray> GenerateTerrain(Heron::TerrainComponent& component);

        // Editing Operations
        static bool Raycast(const Heron::TerrainComponent& terrain, const Heron::TransformComponent& transform, const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& outHitPoint);
        static void Sculpt(Heron::TerrainComponent& terrain, const glm::vec3& localHitPoint, int brushType, float radius, float strength);
        static void UpdateMesh(Heron::TerrainComponent& terrain);

        // Used by physics (legacy/math approach if needed)
        static float GetHeightAt(const Heron::TerrainComponent& component, float localX, float localZ);
    };

}
