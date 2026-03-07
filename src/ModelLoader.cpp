#include "ModelLoader.h"
#include "3DViewer.h"

#include "ModelLoader.h"
#include "3DViewer.h"
#include <iostream>
#include <unordered_map>
#include <limits>
#include <algorithm>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// Ayuda para cambiar el tamaño de la imagen RGBA
static unsigned char* resizeRGBA(const unsigned char* src, int srcW, int srcH, int dstW, int dstH) {
    if (!src || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return nullptr;
    unsigned char* dst = new unsigned char[dstW * dstH * 4];
    for (int y = 0; y < dstH; ++y) {
        float v = (float)y * (float)(srcH) / (float)(dstH);
        int y0 = (int)floor(v);
        int y1 = std::min(y0 + 1, srcH - 1);
        float fy = v - y0;
        for (int x = 0; x < dstW; ++x) {
            float u = (float)x * (float)(srcW) / (float)(dstW);
            int x0 = (int)floor(u);
            int x1 = std::min(x0 + 1, srcW - 1);
            float fx = u - x0;
            for (int c = 0; c < 4; ++c) {
                float c00 = src[(y0 * srcW + x0) * 4 + c];
                float c10 = src[(y0 * srcW + x1) * 4 + c];
                float c01 = src[(y1 * srcW + x0) * 4 + c];
                float c11 = src[(y1 * srcW + x1) * 4 + c];
                float c0 = c00 * (1.0f - fx) + c10 * fx;
                float c1 = c01 * (1.0f - fx) + c11 * fx;
                float cval = c0 * (1.0f - fy) + c1 * fy;
                int out = (int)(cval + 0.5f);
                dst[(y * dstW + x) * 4 + c] = (unsigned char)std::min(255, std::max(0, out));
            }
        }
    }
    return dst;
}

// Carga de textura
unsigned int C3DViewer::loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    std::cout << "Intentando cargar textura desde: " << path << std::endl;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    std::cout << "Cargando textura: " << path << " -> " << (data ? "OK" : "FALLÓ") << std::endl;
    if (!data) {
        std::cerr << "ERROR: No se pudo encontrar la imagen en: " << path << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
    std::cout << "Textura cargada correctamente: " << path << " (" << width << "x" << height << ")" << std::endl;
    GLenum format = GL_RGB;
    if (nrComponents == 1) format = GL_RED;
    else if (nrComponents == 3) format = GL_RGB;
    else if (nrComponents == 4) format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

// Carga de cubemap
unsigned int C3DViewer::loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    int faceWidth = -1, faceHeight = -1;
    int targetFaceSize = -1;
    bool allFacesLoaded = true;
    stbi_set_flip_vertically_on_load(false);
    if (faces.size() != 6) {
        std::cerr << "Warning: cubemap expects 6 faces, got " << faces.size() << std::endl;
    }
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
        if (data) {
            int curMin = std::min(width, height);
            if (targetFaceSize < 0) targetFaceSize = curMin;

            unsigned char* uploadData = data;
            bool allocatedTemp = false;
            if (width != targetFaceSize || height != targetFaceSize) {
                unsigned char* temp = resizeRGBA(data, width, height, targetFaceSize, targetFaceSize);
                if (temp) {
                    uploadData = temp;
                    allocatedTemp = true;
                }
                else {
                    int xoff = (width - targetFaceSize) / 2;
                    int yoff = (height - targetFaceSize) / 2;
                    size_t outRowBytes = (size_t)targetFaceSize * 4;
                    unsigned char* tmp2 = new unsigned char[targetFaceSize * targetFaceSize * 4];
                    for (int r = 0; r < targetFaceSize; ++r) {
                        unsigned char* src = data + ((r + yoff) * width + xoff) * 4;
                        unsigned char* dst = tmp2 + r * outRowBytes;
                        memcpy(dst, src, outRowBytes);
                    }
                    uploadData = tmp2;
                    allocatedTemp = true;
                }
            }

            if (faceWidth < 0) {
                faceWidth = targetFaceSize;
                faceHeight = targetFaceSize;
            }
            else if (targetFaceSize != faceWidth || targetFaceSize != faceHeight) {
                allFacesLoaded = false;
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGBA, targetFaceSize, targetFaceSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, uploadData);
            GLenum gle = glGetError();
            if (gle != GL_NO_ERROR) {
                std::cerr << "GL error after glTexImage2D for face " << i << ": 0x" << std::hex << gle << std::dec << std::endl;
            }

            if (allocatedTemp) delete[] uploadData;
            stbi_image_free(data);
        }
        else {
            allFacesLoaded = false;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    if (!allFacesLoaded) {
        std::cerr << "Warning: cubemap is incomplete or inconsistent; rendering may appear black." << std::endl;
    }

    return textureID;
}

// Cargador de OBJ de malla única
void C3DViewer::loadOBJTo(const std::string& path, GLuint& outVAO, GLuint& outVBO, size_t& outVertexCount, bool& outHasTexCoords, float& outMinY, float& outMaxY, GLuint* outTexture, glm::vec3* outKa, glm::vec3* outKd, glm::vec3* outKs, float* outNs) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::cout << "Intentando cargar modelo desde: " << path << std::endl;

    std::string baseDir = "";
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) baseDir = path.substr(0, pos + 1);
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str())) {
        std::cout << "Error cargando OBJ: " << err << std::endl;
        return;
    }

    std::cout << "Modelo cargado con exito (loadOBJTo)" << std::endl;
    std::cout << "Vertices encontrados: " << attrib.vertices.size() / 3 << std::endl;
    std::cout << "Caras/Formas encontradas: " << shapes.size() << std::endl;
    std::cout << "Materiales cargados: " << materials.size() << std::endl;

    std::vector<Vertex> vertices;
    bool hasTexCoords = false;
    bool hasNormals = false;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            if (index.normal_index >= 0) {
                hasNormals = true;
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
            if (index.texcoord_index >= 0) {
                vertex.TexCoords = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
                hasTexCoords = true;
            }
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
            vertices.push_back(vertex);
        }
    }
    outVertexCount = vertices.size();
    outHasTexCoords = hasTexCoords;

    if (!hasTexCoords) {
        glm::vec3 coffeeColor = glm::vec3(0.9f, 0.8f, 0.7f);
        for (auto& v : vertices) v.Color = coffeeColor;
    }

    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    for (const auto& v : vertices) {
        if (v.Position.y < minY) minY = v.Position.y;
        if (v.Position.y > maxY) maxY = v.Position.y;
    }
    outMinY = minY; outMaxY = maxY;
    std::cout << "Altura del OBJ: minY = " << minY << ", maxY = " << maxY << ", altura total = " << (maxY - minY) << std::endl;

    if (outTexture) {
        *outTexture = 0;
        if (!materials.empty()) {
            int chosenMat = -1;
            int bestCount = -1;
            for (size_t i = 0; i < materials.size(); ++i) {
                if (!materials[i].diffuse_texname.empty()) {
                    int count = 0;
                    for (const auto &shape : shapes) {
                        for (int mid : shape.mesh.material_ids) { if (mid == (int)i) count++; }
                    }
                    if (count > bestCount) { bestCount = count; chosenMat = i; }
                }
            }
            if (chosenMat == -1) {
                std::unordered_map<int,int> counts;
                for (const auto &shape : shapes) for (int mid : shape.mesh.material_ids) counts[mid]++;
                bestCount = -1;
                for (auto &kv : counts) if (kv.second > bestCount && kv.first >= 0) { bestCount = kv.second; chosenMat = kv.first; }
                if (chosenMat == -1) chosenMat = 0;
            }
            if (chosenMat >= 0 && chosenMat < (int)materials.size()) {
                const auto &mat = materials[chosenMat];
                if (!mat.diffuse_texname.empty()) {
                    std::string texPath = baseDir + mat.diffuse_texname;
                    GLuint tid = loadTexture(texPath.c_str());
                    if (tid == 0) {
                        std::cout << "Warning: failed to load diffuse texture '" << texPath << "' for OBJ '" << path << "'" << std::endl;
                    } else {
                        std::cout << "Loaded material diffuse for OBJ: " << texPath << std::endl;
                        *outTexture = tid;
                    }
                }
                if (outKa) *outKa = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                if (outKd) *outKd = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                if (outKs) *outKs = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
                if (outNs) *outNs = (mat.shininess > 0.0f) ? mat.shininess : 32.0f;
            }
        }
    }

    if (!hasNormals && outVertexCount >= 3) {
        for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
            glm::vec3 v0 = vertices[i + 0].Position;
            glm::vec3 v1 = vertices[i + 1].Position;
            glm::vec3 v2 = vertices[i + 2].Position;
            glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            if (glm::length(faceNormal) > 0.0f) {
                vertices[i + 0].Normal += faceNormal;
                vertices[i + 1].Normal += faceNormal;
                vertices[i + 2].Normal += faceNormal;
            }
        }
        for (size_t i = 0; i < vertices.size(); ++i) {
            if (glm::length(vertices[i].Normal) > 0.0001f) vertices[i].Normal = glm::normalize(vertices[i].Normal);
            else vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        std::cout << "Computed vertex normals for OBJ (was missing in file)." << std::endl;
    }

    glGenVertexArrays(1, &outVAO);
    glGenBuffers(1, &outVBO);
    glBindVertexArray(outVAO);
    glBindBuffer(GL_ARRAY_BUFFER, outVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
}

// Cargador de OBJ de malla múltiple
void C3DViewer::loadOBJToMulti(const std::string& path, std::vector<Submesh>& outSubmeshes, float& outMinY, float& outMaxY, bool& outHasTexCoords) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    outSubmeshes.clear();
    std::string baseDir = "";
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) baseDir = path.substr(0, pos + 1);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), baseDir.c_str())) {
        std::cout << "Error cargando OBJ (multi): " << err << std::endl;
        return;
    }

    std::cout << "Materiales cargados: " << materials.size() << std::endl;
    for (size_t i = 0; i < materials.size(); ++i) {
        std::cout << "  Material " << i << ": " << materials[i].name << " diffuse_tex: " << materials[i].diffuse_texname << std::endl;
    }

    bool anyHasTex = false;
    float globalMinY = std::numeric_limits<float>::max();
    float globalMaxY = -std::numeric_limits<float>::max();

    std::vector<GLuint> matAmbientTex(materials.size(), 0);
    std::vector<GLuint> matDiffuseTex(materials.size(), 0);
    std::vector<GLuint> matSpecularTex(materials.size(), 0);
    for (size_t mi = 0; mi < materials.size(); ++mi) {
        const auto &m = materials[mi];
        if (!m.ambient_texname.empty()) {
            std::string p = baseDir + m.ambient_texname;
            matAmbientTex[mi] = loadTexture(p.c_str());
            if (matAmbientTex[mi] == 0) std::cout << "Warning: failed to load ambient map '" << p << "' for material " << m.name << std::endl;
        }
        if (!m.diffuse_texname.empty()) {
            std::string p = baseDir + m.diffuse_texname;
            matDiffuseTex[mi] = loadTexture(p.c_str());
            if (matDiffuseTex[mi] == 0) std::cout << "Warning: failed to load diffuse map '" << p << "' for material " << m.name << std::endl;
        }
        if (!m.specular_texname.empty()) {
            std::string p = baseDir + m.specular_texname;
            matSpecularTex[mi] = loadTexture(p.c_str());
            if (matSpecularTex[mi] == 0) std::cout << "Warning: failed to load specular map '" << p << "' for material " << m.name << std::endl;
        }
    }

    std::vector<glm::vec3> matKa(materials.size(), glm::vec3(0.02f));
    std::vector<glm::vec3> matKd(materials.size(), glm::vec3(0.8f));
    std::vector<glm::vec3> matKs(materials.size(), glm::vec3(0.2f));
    std::vector<float> matNs(materials.size(), 32.0f);

    for (size_t mi = 0; mi < materials.size(); ++mi) {
        const auto &m = materials[mi];
        matKa[mi] = glm::vec3(m.ambient[0], m.ambient[1], m.ambient[2]);
        matKd[mi] = glm::vec3(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
        matKs[mi] = glm::vec3(m.specular[0], m.specular[1], m.specular[2]);
        matNs[mi] = (m.shininess > 0.0f) ? m.shininess : 32.0f;
    }

    for (size_t s = 0; s < shapes.size(); ++s) {
        const auto& shape = shapes[s];
        std::vector<Vertex> vertices;
        bool hasTex = false;
        bool hasNormals = false;

        int chosenMat = -1;
        if (!shape.mesh.material_ids.empty()) {
            std::unordered_map<int,int> counts;
            for (int mid : shape.mesh.material_ids) counts[mid]++;
            int best = -1; int bestCount = -1;
            for (auto &kv : counts) if (kv.second > bestCount) { best = kv.first; bestCount = kv.second; }
            chosenMat = best;
        }
        if (chosenMat == -1 && !materials.empty()) {
            for (size_t i = 0; i < materials.size(); ++i) if (!materials[i].diffuse_texname.empty()) { chosenMat = i; break; }
        }

        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.Position = { attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2] };
            if (index.normal_index >= 0) { hasNormals = true; vertex.Normal = { attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1], attrib.normals[3 * index.normal_index + 2] }; }
            if (index.texcoord_index >= 0) { hasTex = true; vertex.TexCoords = { attrib.texcoords[2 * index.texcoord_index + 0], attrib.texcoords[2 * index.texcoord_index + 1] }; }
            vertex.Color = glm::vec3(1.0f);
            vertices.push_back(vertex);
        }

        if (!hasNormals && vertices.size() >= 3) {
            std::vector<glm::vec3> accumulatedNormals(vertices.size(), glm::vec3(0.0f));
            for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
                glm::vec3 v0 = vertices[i + 0].Position;
                glm::vec3 v1 = vertices[i + 1].Position;
                glm::vec3 v2 = vertices[i + 2].Position;
                glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                if (glm::length(faceNormal) > 0.0001f) { accumulatedNormals[i + 0] += faceNormal; accumulatedNormals[i + 1] += faceNormal; accumulatedNormals[i + 2] += faceNormal; }
            }
            for (size_t i = 0; i < vertices.size(); ++i) { if (glm::length(accumulatedNormals[i]) > 0.0001f) vertices[i].Normal = glm::normalize(accumulatedNormals[i]); else vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f); }
        }

        if (chosenMat == -1) {
            static const glm::vec3 cardColors[] = { glm::vec3(1.0f,0.8f,0.8f), glm::vec3(0.8f,1.0f,0.8f), glm::vec3(0.8f,0.8f,1.0f), glm::vec3(1.0f,1.0f,0.8f), glm::vec3(1.0f,0.8f,1.0f), glm::vec3(0.8f,1.0f,1.0f) };
            glm::vec3 color = cardColors[s % (sizeof(cardColors) / sizeof(cardColors[0]))];
            for (auto& v : vertices) v.Color = color;
        }

        float minY = std::numeric_limits<float>::max(); float maxY = -std::numeric_limits<float>::max();
        for (const auto &v : vertices) { if (v.Position.y < minY) minY = v.Position.y; if (v.Position.y > maxY) maxY = v.Position.y; }
        if (minY < globalMinY) globalMinY = minY;
        if (maxY > globalMaxY) globalMaxY = maxY;

        Submesh sm;
        sm.vertexCount = vertices.size(); sm.hasTexCoords = hasTex; sm.minY = minY; sm.maxY = maxY; sm.materialId = chosenMat; sm.name = shape.name;

        glGenVertexArrays(1, &sm.vao);
        glGenBuffers(1, &sm.vbo);
        glBindVertexArray(sm.vao);
        glBindBuffer(GL_ARRAY_BUFFER, sm.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
        glEnableVertexAttribArray(3); glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        if (chosenMat >= 0 && chosenMat < (int)materials.size()) {
            sm.texAmbient = matAmbientTex[chosenMat]; sm.texDiffuse = matDiffuseTex[chosenMat]; sm.texSpecular = matSpecularTex[chosenMat];
            sm.Ka = matKa[chosenMat]; sm.Kd = matKd[chosenMat]; sm.Ks = matKs[chosenMat]; sm.Ns = matNs[chosenMat];
            if (sm.texDiffuse == 0 && sm.hasTexCoords) {
                const auto &mat = materials[chosenMat]; if (!mat.diffuse_texname.empty()) { std::string p = baseDir + mat.diffuse_texname; sm.texDiffuse = loadTexture(p.c_str()); }
            }
        }

        std::cout << "Submesh loaded: '" << sm.name << "' material=" << sm.materialId << " hasTexCoords=" << (sm.hasTexCoords?"yes":"no") << " texDiffuse=" << sm.texDiffuse << " texAmbient=" << sm.texAmbient << " texSpecular=" << sm.texSpecular << std::endl;
        outSubmeshes.push_back(sm);
    }

    outMinY = globalMinY; outMaxY = globalMaxY; outHasTexCoords = false; for (auto &s : outSubmeshes) if (s.hasTexCoords) { outHasTexCoords = true; break; }

    float totalHeight = outMaxY - outMinY;
    for (size_t i = 0; i < outSubmeshes.size(); ++i) {
        float centroidY = (outSubmeshes[i].minY + outSubmeshes[i].maxY) * 0.5f;
        outSubmeshes[i].isFruit = (centroidY > outMinY + totalHeight * 0.25f);
    }

    std::cout << "Loaded multi-shape OBJ: shapes=" << outSubmeshes.size() << " globalMinY=" << outMinY << " globalMaxY=" << outMaxY << std::endl;
}

// Carga de mesa OBJ
void C3DViewer::loadOBJ(const std::string& path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::cout << "Intentando cargar modelo desde: " << path << std::endl;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), "OBJs/table/")) {
        std::cout << "Error cargando OBJ: " << err << std::endl;
        return;
    }

    std::cout << "Modelo cargado con exito (loadOBJ)" << std::endl;
    std::cout << "Vertices encontrados: " << attrib.vertices.size() / 3 << std::endl;
    std::cout << "Caras/Formas encontradas: " << shapes.size() << std::endl;
    std::cout << "Materiales cargados: " << materials.size() << std::endl;

    std::vector<Vertex> vertices;
    bool hasTexCoords = false;
    bool hasNormals = false;
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            if (index.normal_index >= 0) {
                hasNormals = true;
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
            if (index.texcoord_index >= 0) {
                vertex.TexCoords = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
                hasTexCoords = true;
            }
            // Por defecto asignamos color blanco (si el OBJ no trae color por vértice)
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
            vertices.push_back(vertex);
        }
    }
    m_tableVertexCount = vertices.size();
    m_tableHasTexCoords = hasTexCoords;

    // Calculo de los límites en Y
    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    for (const auto& v : vertices) {
        if (v.Position.y < minY) minY = v.Position.y;
        if (v.Position.y > maxY) maxY = v.Position.y;
    }
    // Calculo de normales si el OBJ no las proporciona
    if (!hasNormals && vertices.size() >= 3) {
        for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
            glm::vec3 v0 = vertices[i + 0].Position;
            glm::vec3 v1 = vertices[i + 1].Position;
            glm::vec3 v2 = vertices[i + 2].Position;
            glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            if (glm::length(faceNormal) > 0.0001f) {
                vertices[i + 0].Normal += faceNormal;
                vertices[i + 1].Normal += faceNormal;
                vertices[i + 2].Normal += faceNormal;
            }
        }
        for (size_t i = 0; i < vertices.size(); ++i) {
            if (glm::length(vertices[i].Normal) > 0.0001f)
                vertices[i].Normal = glm::normalize(vertices[i].Normal);
            else
                vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // Creacion de búfer de GPU
    m_tableMaxY = maxY;
    std::cout << "Altura de la mesa: minY = " << minY << ", maxY = " << maxY << ", altura total = " << (maxY - minY) << std::endl;

    // Calculo de vertices de normales si no se proporcionan en el OBJ
    if (!hasNormals && m_tableVertexCount >= 3) {
        for (size_t i = 0; i + 2 < vertices.size(); i += 3) {
            glm::vec3 v0 = vertices[i + 0].Position;
            glm::vec3 v1 = vertices[i + 1].Position;
            glm::vec3 v2 = vertices[i + 2].Position;
            glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            if (glm::length(faceNormal) > 0.0f) {
                vertices[i + 0].Normal += faceNormal;
                vertices[i + 1].Normal += faceNormal;
                vertices[i + 2].Normal += faceNormal;
            }
        }
        for (size_t i = 0; i < vertices.size(); ++i) {
            if (glm::length(vertices[i].Normal) > 0.0001f)
                vertices[i].Normal = glm::normalize(vertices[i].Normal);
            else
                vertices[i].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        std::cout << "Computed vertex normals for OBJ (was missing in file)." << std::endl;
    }
    if (m_tableHasTexCoords) {
        std::cout << "OBJ contiene UVs. Primeros TexCoords: ";
        if (!vertices.empty()) {
            std::cout << vertices[0].TexCoords.x << "," << vertices[0].TexCoords.y << std::endl;
        }
    }
    else {
        std::cout << "OBJ no contiene UVs." << std::endl;
    }
    std::cout << "Vertices totales procesados para dibujo: " << m_tableVertexCount << std::endl;

    glGenVertexArrays(1, &m_tableVAO);
    glGenBuffers(1, &m_tableVBO);
    glBindVertexArray(m_tableVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_tableVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Posicion
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // Normales
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // Color (default blanco para OBJ)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    // UVs
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
}