#include "mesh.hpp"

#include <cmath>
#include <iostream>

#include <glm/gtc/constants.hpp>


std::unordered_map<std::string, std::shared_ptr<Mesh>> Mesh::meshCache;



Mesh::Mesh(){}



Mesh::~Mesh(){}



void Mesh::clearMeshCache() {
    std::cout << "Vidage du cache de meshes (" << meshCache.size() << " entrées)" << std::endl;
    meshCache.clear();
}

std::shared_ptr<Mesh> Mesh::getZombieMesh() {
    if (meshCache.find("zombie") != meshCache.end()) {
        return meshCache["zombie"];
    }

    auto zombieMesh = std::make_shared<Mesh>();

    zombieMesh->addBox(glm::vec3(0.0f, 0.6f, 0.0f), glm::vec3(0.6f, 0.8f, 0.4f));

    zombieMesh->addBox(glm::vec3(0.0f, 1.2f, 0.0f), glm::vec3(0.4f, 0.4f, 0.4f));

    meshCache["zombie"] = zombieMesh;
    std::cout << "Mesh du zombie genere et mis en cache !" << std::endl;

    return zombieMesh;
}

void Mesh::addBox(const glm::vec3& c, const glm::vec3& s) {
    float x = c.x, y = c.y, z = c.z;
    float hx = s.x * 0.5f, hy = s.y * 0.5f, hz = s.z * 0.5f;

    unsigned int startVertex = this->vertices.size();

    struct VertexData {
        glm::vec3 p; glm::vec3 n; glm::vec2 u; glm::vec3 t; glm::vec3 b;
    };

    std::vector<VertexData> faceVertices = {

        {{x-hx, y-hy, z+hz}, {0,0,1}, {0,0}, {1,0,0}, {0,1,0}},
        {{x+hx, y-hy, z+hz}, {0,0,1}, {1,0}, {1,0,0}, {0,1,0}},
        {{x+hx, y+hy, z+hz}, {0,0,1}, {1,1}, {1,0,0}, {0,1,0}},
        {{x-hx, y+hy, z+hz}, {0,0,1}, {0,1}, {1,0,0}, {0,1,0}},

        {{x+hx, y-hy, z-hz}, {0,0,-1}, {0,0}, {-1,0,0}, {0,1,0}},
        {{x-hx, y-hy, z-hz}, {0,0,-1}, {1,0}, {-1,0,0}, {0,1,0}},
        {{x-hx, y+hy, z-hz}, {0,0,-1}, {1,1}, {-1,0,0}, {0,1,0}},
        {{x+hx, y+hy, z-hz}, {0,0,-1}, {0,1}, {-1,0,0}, {0,1,0}},

        {{x-hx, y-hy, z-hz}, {-1,0,0}, {0,0}, {0,0,1}, {0,1,0}},
        {{x-hx, y-hy, z+hz}, {-1,0,0}, {1,0}, {0,0,1}, {0,1,0}},
        {{x-hx, y+hy, z+hz}, {-1,0,0}, {1,1}, {0,0,1}, {0,1,0}},
        {{x-hx, y+hy, z-hz}, {-1,0,0}, {0,1}, {0,0,1}, {0,1,0}},

        {{x+hx, y-hy, z+hz}, {1,0,0}, {0,0}, {0,0,-1}, {0,1,0}},
        {{x+hx, y-hy, z-hz}, {1,0,0}, {1,0}, {0,0,-1}, {0,1,0}},
        {{x+hx, y+hy, z-hz}, {1,0,0}, {1,1}, {0,0,-1}, {0,1,0}},
        {{x+hx, y+hy, z+hz}, {1,0,0}, {0,1}, {0,0,-1}, {0,1,0}},

        {{x-hx, y+hy, z+hz}, {0,1,0}, {0,0}, {1,0,0}, {0,0,-1}},
        {{x+hx, y+hy, z+hz}, {0,1,0}, {1,0}, {1,0,0}, {0,0,-1}},
        {{x+hx, y+hy, z-hz}, {0,1,0}, {1,1}, {1,0,0}, {0,0,-1}},
        {{x-hx, y+hy, z-hz}, {0,1,0}, {0,1}, {1,0,0}, {0,0,-1}},

        {{x-hx, y-hy, z-hz}, {0,-1,0}, {0,0}, {1,0,0}, {0,0,1}},
        {{x+hx, y-hy, z-hz}, {0,-1,0}, {1,0}, {1,0,0}, {0,0,1}},
        {{x+hx, y-hy, z+hz}, {0,-1,0}, {1,1}, {1,0,0}, {0,0,1}},
        {{x-hx, y-hy, z+hz}, {0,-1,0}, {0,1}, {1,0,0}, {0,0,1}}
    };

    for(const auto& v : faceVertices) {
        this->vertices.push_back(v.p);
        this->normals.push_back(v.n);
        this->uvs.push_back(v.u);
        this->tangents.push_back(v.t);
        this->bitangents.push_back(v.b);
    }

    for (unsigned int i = 0; i < 6; ++i) {
        unsigned int b = startVertex + i * 4;
        this->indices.push_back(b + 0);
        this->indices.push_back(b + 1);
        this->indices.push_back(b + 2);
        this->indices.push_back(b + 0);
        this->indices.push_back(b + 2);
        this->indices.push_back(b + 3);
    }
}