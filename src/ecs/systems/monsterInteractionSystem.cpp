#include "monsterInteractionSystem.hpp"

void MonsterInteractionSystem::update(Registry& _registry, float _deltaTime) {

    auto inputView = _registry.view<InputReceiverComponent>();
    bool spawnRequested = false;

    for (EntityID entity : inputView) {
        if (_registry.getComponent<InputReceiverComponent>(entity).toggleMonsterSpawn) {
            spawnRequested = true;
            break;
        }
    }

    auto playerView = _registry.view<PlayerComponent, TransformComponent>();
    bool hasPlayer = (playerView.begin() != playerView.end());

    if (spawnRequested && hasPlayer) {
        auto monsterView = _registry.view<MonsterComponent>();
        
        if (monsterView.begin() == monsterView.end()) {
            EntityID playerEntity = *playerView.begin();
            const auto& playerTransform = _registry.getComponent<TransformComponent>(playerEntity);

            glm::vec3 spawnPosition = playerTransform.position + glm::vec3(3.0f, 0.0f, 3.0f);

            EntityID zombie = _registry.createEntity();
            _registry.addComponent(zombie, MonsterComponent{});
            _registry.addComponent(zombie, IAComponent{});
            _registry.addComponent(zombie, TransformComponent{ spawnPosition, glm::vec3(0.0f) });
            _registry.addComponent(zombie, VelocityComponent{ .movementSpeed = 3.0f });
            _registry.addComponent(zombie, RigidBodyComponent{});
            _registry.addComponent(zombie, ColliderComponent{ glm::vec3(0.8f, 1.8f, 0.8f), glm::vec3(0.0f, 0.9f, 0.0f) });

            auto zombieData = Mesh::getZombieMesh();

            std::vector<glm::vec3> uvs3D;
            uvs3D.reserve(zombieData->uvs.size());
            
            float dirtSliceIndex = 1.0f; 
            for (const auto& uv2D : zombieData->uvs) {
                uvs3D.push_back(glm::vec3(uv2D.x, uv2D.y, dirtSliceIndex));
            }

            GLuint VAO, VBO_pos, VBO_normal, VBO_uv, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO_pos);
            glGenBuffers(1, &VBO_normal);
            glGenBuffers(1, &VBO_uv);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO_pos);
            glBufferData(GL_ARRAY_BUFFER, zombieData->vertices.size() * sizeof(glm::vec3), zombieData->vertices.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
            glBufferData(GL_ARRAY_BUFFER, zombieData->normals.size() * sizeof(glm::vec3), zombieData->normals.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, VBO_uv);
            glBufferData(GL_ARRAY_BUFFER, uvs3D.size() * sizeof(glm::vec3), uvs3D.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(2);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, zombieData->indices.size() * sizeof(unsigned int), zombieData->indices.data(), GL_STATIC_DRAW);

            glBindVertexArray(0);

            MeshComponent zombieMesh;
            zombieMesh.VAO = VAO;
            zombieMesh.indexCount = zombieData->indices.size();

            _registry.addComponent(zombie, zombieMesh);

            std::cout << "[ECS] Zombie en terre généré avec Texture Array ID (Slice 1).\n";
        } else {
            std::vector<EntityID> monstersToDestroy;
            for (EntityID monster : monsterView) {
                monstersToDestroy.push_back(monster);
            }

            for (EntityID monster : monstersToDestroy) {
                if (_registry.hasComponent<MeshComponent>(monster)) {
                    const auto& mesh = _registry.getComponent<MeshComponent>(monster);
                    if (mesh.VAO != 0) {
                        glDeleteVertexArrays(1, &mesh.VAO);
                    }
                }
                _registry.destroyEntity(monster);
            }
            std::cout << "[ECS] Zombie(s) détruit(s) (Toggle OFF).\n";
        }
    }

    if (!hasPlayer) return;
    
    EntityID playerEntity = *playerView.begin();
    const auto& playerTransform = _registry.getComponent<TransformComponent>(playerEntity);

    auto monsterView = _registry.view<MonsterComponent, IAComponent, TransformComponent>();
    for (EntityID monster : monsterView) {
        auto& ai = _registry.getComponent<IAComponent>(monster);
        auto& monsterComponent = _registry.getComponent<MonsterComponent>(monster);
        const auto& transform = _registry.getComponent<TransformComponent>(monster);

        float distance = glm::distance(transform.position, playerTransform.position);

        if (!monsterComponent.isChasing) {
            if (distance <= monsterComponent.detectionRange) {
                monsterComponent.isChasing = true;
                std::cout << "[IA] Zombie provoqué ! Chasse activée.\n";
            } else {
                glm::ivec3 currentPos(
                    std::floor(transform.position.x),
                    std::floor(transform.position.y),
                    std::floor(transform.position.z)
                );

                if (ai.currentPath.empty()) {
                    if (monsterComponent.currentWanderingTimer > 0.0f) {
                        monsterComponent.currentWanderingTimer -= _deltaTime;
                        if (monsterComponent.currentWanderingTimer < 0.0f) {
                            monsterComponent.currentWanderingTimer = 0.0f; 
                        }
                    } 
                    else if (monsterComponent.currentWanderingTimer == 0.0f) {
                        int range = static_cast<int>(monsterComponent.wanderingRange);
                        int rx = (std::rand() % (range * 2 + 1)) - range;
                        int rz = (std::rand() % (range * 2 + 1)) - range;
                        int ry = (std::rand() % 3) - 1; 

                        if (rx != 0 || rz != 0) {
                            glm::ivec3 newWanderTarget = currentPos + glm::ivec3(rx, ry, rz);
                            ai.target = newWanderTarget;
                            ai.currentPath.clear();

                            monsterComponent.currentWanderingTimer = -0.1f;
                        }
                    } 
                    else {
                        monsterComponent.currentWanderingTimer += _deltaTime;
                        if (monsterComponent.currentWanderingTimer >= 0.0f) {
                            monsterComponent.currentWanderingTimer = 0.0f;
                        }
                    }
                } else {
                    monsterComponent.currentWanderingTimer = monsterComponent.wanderingTimer;
                }
            }
        } else {
            if (distance > monsterComponent.loseTargetRange) {
                monsterComponent.isChasing = false;
                
                ai.currentPath.clear(); 
                ai.target = glm::ivec3(std::floor(transform.position.x), 
                                       std::floor(transform.position.y), 
                                       std::floor(transform.position.z));
                                       
                monsterComponent.currentWanderingTimer = monsterComponent.wanderingTimer;
                
                std::cout << "[IA] Le joueur est trop loin. Le zombie s'arrête.\n";
            }
        }

        if (monsterComponent.isChasing) {
            glm::ivec3 newTarget(std::floor(playerTransform.position.x), 
                                 std::floor(playerTransform.position.y), 
                                 std::floor(playerTransform.position.z));

            if (ai.target != newTarget) {
                ai.target = newTarget;
                ai.currentPath.clear();
            }
        }

        if (distance <= monsterComponent.attackRange) {
            // pas encore implémenté
        }
    }
}