#pragma once

#include "Heron/Renderer/VertexArray.h"
#include <string>
#include <memory>

namespace Heron {

    class TerrainGenerator {
    public:
        // Generates a terrain mesh based on a heightmap.
        // heightmapPath: path to a grayscale image (png, jpg, etc.)
        // size: world size of the terrain (width and depth in units)
        // heightScale: world height scaling multiplier for the heightmap values (0.0 to 1.0)
        static Ref<VertexArray> GenerateTerrain(struct TerrainComponent& terrain);
        
        // Samples the height at a specific local coordinate
        static float GetHeightAt(const struct TerrainComponent& terrain, float localX, float localZ);
    };

}
