#include "hrnpch.h"
#include "TerrainGenerator.h"
#include "Heron/Renderer/Buffer.h"
#include "Heron/Scene/Components.h"

#include <stb_image.h>
#include <vector>
#include <glm/glm.hpp>

namespace Heron {

    Ref<VertexArray> TerrainGenerator::GenerateTerrain(TerrainComponent& terrain) {
        
        int width = 1, height = 1, channels = 1;
        unsigned char* data = nullptr;

        terrain.HeightmapData.clear();
        terrain.HeightmapWidth = 0;
        terrain.HeightmapHeight = 0;

        if (!terrain.HeightmapPath.empty()) {
            stbi_set_flip_vertically_on_load(1);
            data = stbi_load(terrain.HeightmapPath.c_str(), &width, &height, &channels, 1); // load as 1 channel (grayscale)
            
            if (data) {
                terrain.HeightmapWidth = width;
                terrain.HeightmapHeight = height;
                terrain.HeightmapData.assign(data, data + (width * height));
            }
        }
        
        int resolution = terrain.Resolution;
        float size = terrain.Size;
        float heightScale = terrain.HeightScale;

        int vertexCount = (resolution + 1) * (resolution + 1);
        std::vector<float> vertices;
        vertices.reserve(vertexCount * 8); // x, y, z, nx, ny, nz, u, v

        float halfSize = size / 2.0f;
        float dx = size / (float)resolution;
        float dz = size / (float)resolution;

        // Generate Vertices
        for (int z = 0; z <= resolution; ++z) {
            for (int x = 0; x <= resolution; ++x) {
                // World coordinates (starting from 0,0,0 to match Box3D HeightField)
                float px = x * dx;
                float pz = z * dz;
                
                // Heightmap sampling
                float py = 0.0f;
                if (data) {
                    // Map x,z to image coordinates
                    float u = (float)x / (float)resolution;
                    float v = (float)z / (float)resolution;
                    
                    int texX = (int)(u * (width - 1));
                    int texY = (int)(v * (height - 1));
                    
                    // Safe clamp
                    if (texX < 0) texX = 0;
                    if (texX >= width) texX = width - 1;
                    if (texY < 0) texY = 0;
                    if (texY >= height) texY = height - 1;

                    unsigned char pixelValue = data[texY * width + texX];
                    float normalizedHeight = (float)pixelValue / 255.0f;
                    py = normalizedHeight * heightScale;
                }

                // Temporary normal (will recalculate later if needed)
                float nx = 0.0f, ny = 1.0f, nz = 0.0f;
                
                // UV coordinates
                float u = (float)x / (float)resolution;
                float v = (float)z / (float)resolution;

                vertices.push_back(px);
                vertices.push_back(py);
                vertices.push_back(pz);
                
                vertices.push_back(nx);
                vertices.push_back(ny);
                vertices.push_back(nz);
                
                vertices.push_back(u);
                vertices.push_back(v);
            }
        }

        // Generate Indices (Triangles)
        std::vector<uint32_t> indices;
        int numQuads = resolution * resolution;
        indices.reserve(numQuads * 6);

        for (int z = 0; z < resolution; ++z) {
            for (int x = 0; x < resolution; ++x) {
                int topLeft = (z * (resolution + 1)) + x;
                int topRight = topLeft + 1;
                int bottomLeft = ((z + 1) * (resolution + 1)) + x;
                int bottomRight = bottomLeft + 1;

                // Triangle 1
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // Triangle 2
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        // Free image data
        if (data) {
            stbi_image_free(data);
        }

        // Create Vertex Array
        Ref<VertexArray> vertexArray = VertexArray::Create();

        Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(float));
        BufferLayout layout = {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Normal" },
            { ShaderDataType::Float2, "a_TexCoord" }
        };
        vertexBuffer->SetLayout(layout);
        vertexArray->AddVertexBuffer(vertexBuffer);

        Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices.data(), indices.size());
        vertexArray->SetIndexBuffer(indexBuffer);

        return vertexArray;
    }

    float TerrainGenerator::GetHeightAt(const TerrainComponent& terrain, float localX, float localZ) {
        if (terrain.HeightmapData.empty() || terrain.HeightmapWidth == 0 || terrain.HeightmapHeight == 0) {
            return 0.0f;
        }

        float halfSize = terrain.Size / 2.0f;
        
        // Map local coords to 0.0 - 1.0 UV space
        float u = (localX + halfSize) / terrain.Size;
        float v = (localZ + halfSize) / terrain.Size;

        // Clamp to edge
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;

        // Map to pixel coords
        float texX = u * (terrain.HeightmapWidth - 1);
        float texY = v * (terrain.HeightmapHeight - 1);

        // Bilinear interpolation could be added here for smoother walking, 
        // but for now nearest neighbor is fast and sufficient for a basic system.
        int ix = (int)std::round(texX);
        int iy = (int)std::round(texY);

        // Clamp again just to be safe
        if (ix < 0) ix = 0;
        if (ix >= terrain.HeightmapWidth) ix = terrain.HeightmapWidth - 1;
        if (iy < 0) iy = 0;
        if (iy >= terrain.HeightmapHeight) iy = terrain.HeightmapHeight - 1;

        unsigned char pixelValue = terrain.HeightmapData[iy * terrain.HeightmapWidth + ix];
        float normalizedHeight = (float)pixelValue / 255.0f;

        return normalizedHeight * terrain.HeightScale;
    }

}
