#include "hrnpch.h"
#include "TerrainGenerator.h"
#include "Heron/Renderer/Buffer.h"
#include "Heron/Scene/Components.h"

#include <stb_image.h>
#include <vector>
#include <glm/glm.hpp>

namespace Heron {

    Ref<VertexArray> TerrainGenerator::GenerateTerrain(TerrainComponent& terrain) {
        
        // Initialize RuntimeHeightData if it's empty
        if (terrain.RuntimeHeightData.empty()) {
            int width = 0, height = 0, channels = 1;
            unsigned char* data = nullptr;

            if (!terrain.HeightmapPath.empty()) {
                stbi_set_flip_vertically_on_load(1);
                data = stbi_load(terrain.HeightmapPath.c_str(), &width, &height, &channels, 1);
            }
            
            if (!data || width == 0 || height == 0) {
                width = terrain.Resolution + 1;
                height = terrain.Resolution + 1;
            }

            terrain.HeightmapWidth = width;
            terrain.HeightmapHeight = height;
            terrain.RuntimeHeightData.assign(width * height, 0.0f);

            if (data) {
                for(int i=0; i < width * height; i++) {
                    terrain.RuntimeHeightData[i] = (data[i] / 255.0f) * terrain.HeightScale;
                }
                stbi_image_free(data);
            }
        }
        
        int resolution = terrain.Resolution;
        float size = terrain.Size;

        int vertexCount = (resolution + 1) * (resolution + 1);
        std::vector<float> vertices;
        vertices.reserve(vertexCount * 8); // x, y, z, nx, ny, nz, u, v

        float dx = size / (float)resolution;
        float dz = size / (float)resolution;

        // Generate Vertices
        for (int z = 0; z <= resolution; ++z) {
            for (int x = 0; x <= resolution; ++x) {
                float px = x * dx;
                float pz = z * dz;
                
                // Sample from RuntimeHeightData
                float py = GetHeightAt(terrain, px, pz);

                // Temporary normal
                float nx = 0.0f, ny = 1.0f, nz = 0.0f;
                
                // UV coordinates
                float u = ((float)x / (float)resolution) * terrain.TextureTiling;
                float v = ((float)z / (float)resolution) * terrain.TextureTiling;

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

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
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

        // Recalculate Normals (basic)
        // For simplicity, normals are set to (0,1,0) above. We will let UpdateMesh handle proper normal recalculation if needed, 
        // or we can calculate it here. For now, flat is okay, UpdateMesh has proper normals.

        return vertexArray;
    }

    float TerrainGenerator::GetHeightAt(const TerrainComponent& terrain, float localX, float localZ) {
        if (terrain.RuntimeHeightData.empty() || terrain.HeightmapWidth == 0 || terrain.HeightmapHeight == 0) {
            return 0.0f;
        }

        // Map local coords to 0.0 - 1.0 UV space (Terrain now starts at 0,0,0)
        float u = localX / terrain.Size;
        float v = localZ / terrain.Size;

        // Clamp to edge
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;

        // Map to pixel coords
        float texX = u * (terrain.HeightmapWidth - 1);
        float texY = v * (terrain.HeightmapHeight - 1);

        int ix = (int)std::round(texX);
        int iy = (int)std::round(texY);

        if (ix < 0) ix = 0;
        if (ix >= terrain.HeightmapWidth) ix = terrain.HeightmapWidth - 1;
        if (iy < 0) iy = 0;
        if (iy >= terrain.HeightmapHeight) iy = terrain.HeightmapHeight - 1;

        return terrain.RuntimeHeightData[iy * terrain.HeightmapWidth + ix];
    }
    
    bool TerrainGenerator::Raycast(const Heron::TerrainComponent& terrain, const Heron::TransformComponent& transform, const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& outHitPoint)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(transform.Translation[0], transform.Translation[1], transform.Translation[2]));
        model = glm::rotate(model, transform.Rotation[0], glm::vec3(1, 0, 0));
        model = glm::rotate(model, transform.Rotation[1], glm::vec3(0, 1, 0));
        model = glm::rotate(model, transform.Rotation[2], glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(transform.Scale[0], transform.Scale[1], transform.Scale[2]));

        glm::mat4 invModel = glm::inverse(model);
        glm::vec3 localOrigin = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
        glm::vec3 localDir = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

        float t = 0.0f;
        float step = terrain.Size / (float)terrain.Resolution;
        if (step < 0.1f) step = 0.1f;
        const float maxDist = 2000.0f;
        
        float tMin = 0.0f;
        for (int i=0; i < 4000; i++) {
            glm::vec3 pos = localOrigin + localDir * t;
            
            if (pos.x >= 0.0f && pos.x <= terrain.Size &&
                pos.z >= 0.0f && pos.z <= terrain.Size) 
            {
                float ty = GetHeightAt(terrain, pos.x, pos.z);
                if (pos.y <= ty) {
                    float t0 = t - step;
                    float t1 = t;
                    for (int j=0; j<8; j++) {
                        float tm = (t0 + t1) * 0.5f;
                        glm::vec3 pm = localOrigin + localDir * tm;
                        float tym = GetHeightAt(terrain, pm.x, pm.z);
                        if (pm.y <= tym) t1 = tm;
                        else t0 = tm;
                    }
                    outHitPoint = localOrigin + localDir * t1;
                    return true;
                }
            }
            t += step;
            if (t > maxDist) break;
        }
        return false;
    }

    void TerrainGenerator::Sculpt(Heron::TerrainComponent& terrain, const glm::vec3& localHitPoint, int brushType, float radius, float strength)
    {
        if (terrain.RuntimeHeightData.empty()) return;
        
        float cellSizeX = terrain.Size / (float)(terrain.HeightmapWidth - 1);
        float cellSizeZ = terrain.Size / (float)(terrain.HeightmapHeight - 1);
        
        int hitX = (int)(localHitPoint.x / cellSizeX);
        int hitZ = (int)(localHitPoint.z / cellSizeZ);
        
        int radiusCellsX = (int)(radius / cellSizeX) + 1;
        int radiusCellsZ = (int)(radius / cellSizeZ) + 1;
        
        int minX = std::max(0, hitX - radiusCellsX);
        int maxX = std::min(terrain.HeightmapWidth - 1, hitX + radiusCellsX);
        int minZ = std::max(0, hitZ - radiusCellsZ);
        int maxZ = std::min(terrain.HeightmapHeight - 1, hitZ + radiusCellsZ);
        
        float centerHeight = terrain.RuntimeHeightData[hitZ * terrain.HeightmapWidth + hitX];
        
        for (int z = minZ; z <= maxZ; z++) {
            for (int x = minX; x <= maxX; x++) {
                float dist = glm::length(glm::vec2((x - hitX) * cellSizeX, (z - hitZ) * cellSizeZ));
                if (dist <= radius) {
                    float falloff = 1.0f - (dist / radius);
                    falloff = falloff * falloff;
                    
                    float effect = strength * falloff;
                    int idx = z * terrain.HeightmapWidth + x;
                    
                    if (brushType == 0) {
                        terrain.RuntimeHeightData[idx] += effect;
                    } else if (brushType == 1) {
                        terrain.RuntimeHeightData[idx] -= effect;
                    } else if (brushType == 2) {
                        terrain.RuntimeHeightData[idx] = glm::mix(terrain.RuntimeHeightData[idx], centerHeight, effect);
                    } else if (brushType == 3) {
                        float sum = 0;
                        int count = 0;
                        for (int sz = std::max(0, z-1); sz <= std::min(terrain.HeightmapHeight-1, z+1); sz++) {
                            for (int sx = std::max(0, x-1); sx <= std::min(terrain.HeightmapWidth-1, x+1); sx++) {
                                sum += terrain.RuntimeHeightData[sz * terrain.HeightmapWidth + sx];
                                count++;
                            }
                        }
                        float avg = sum / (float)count;
                        terrain.RuntimeHeightData[idx] = glm::mix(terrain.RuntimeHeightData[idx], avg, effect);
                    }
                }
            }
        }
    }

    void TerrainGenerator::UpdateMesh(Heron::TerrainComponent& terrain)
    {
        terrain.Mesh = GenerateTerrain(terrain);
    }
}
