#pragma once

#include "entity.hpp"
#include "components/component.hpp"
#include <unordered_map>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <memory>
#include <stdexcept>
#include <limits>

// Sparse set storage for a single component type
class ComponentStorage {
public:
    virtual ~ComponentStorage() = default;
    virtual void removeEntity(EntityID entity) = 0;
    virtual bool hasEntity(EntityID entity) const = 0;
};

template <typename T>
class SparseSet : public ComponentStorage {
    static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");

private:
    std::vector<EntityID> dense;
    std::unordered_map<EntityID, size_t> sparse;
    std::vector<T> components;

public:
    SparseSet() = default;

    void add(EntityID entity, const T& component) {
        auto it = sparse.find(entity);
        if (it != sparse.end()) {
            // Already exists, replace
            size_t idx = it->second;
            components[idx] = component;
        } else {
            // New entity
            size_t idx = dense.size();
            dense.push_back(entity);
            sparse[entity] = idx;
            components.push_back(component);
        }
    }

    void removeEntity(EntityID entity) override {
        auto it = sparse.find(entity);
        if (it != sparse.end()) {
            size_t idx = it->second;
            size_t lastIdx = dense.size() - 1;
            EntityID lastEntity = dense[lastIdx];

            // Swap and pop
            dense[idx] = lastEntity;
            sparse[lastEntity] = idx;
            components[idx] = components[lastIdx];

            dense.pop_back();
            components.pop_back();
            sparse.erase(it);
        }
    }

    bool hasEntity(EntityID entity) const override {
        return sparse.find(entity) != sparse.end();
    }

    T& get(EntityID entity) {
        auto it = sparse.find(entity);
        if (it == sparse.end()) {
            throw std::out_of_range("Entity does not have this component");
        }
        return components[it->second];
    }

    const T& get(EntityID entity) const {
        auto it = sparse.find(entity);
        if (it == sparse.end()) {
            throw std::out_of_range("Entity does not have this component");
        }
        return components[it->second];
    }

    const std::vector<EntityID>& getDense() const { return dense; }
    const std::vector<T>& getComponents() const { return components; }
    std::vector<T>& getComponents() { return components; }
    size_t size() const { return dense.size(); }
    bool empty() const { return dense.empty(); }
};

// Main Registry managing all entities and components
class Registry {
private:
    EntityID nextEntity = 1;
    std::unordered_map<std::type_index, std::shared_ptr<ComponentStorage>> storages;

    template <typename T>
    SparseSet<T>* getStorage() {
        auto it = storages.find(std::type_index(typeid(T)));
        if (it == storages.end()) {
            auto storage = std::make_shared<SparseSet<T>>();
            storages[std::type_index(typeid(T))] = storage;
            return static_cast<SparseSet<T>*>(storage.get());
        }
        return static_cast<SparseSet<T>*>(it->second.get());
    }

    template <typename T>
    const SparseSet<T>* getStorageConst() const {
        auto it = storages.find(std::type_index(typeid(T)));
        if (it == storages.end()) {
            return nullptr;
        }
        return static_cast<const SparseSet<T>*>(it->second.get());
    }

public:
    Registry() = default;

    EntityID createEntity() {
        return nextEntity++;
    }

    void destroyEntity(EntityID entity) {
        for (auto& pair : storages) {
            pair.second->removeEntity(entity);
        }
    }

    template <typename T>
    void addComponent(EntityID entity, const T& component) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        auto storage = getStorage<T>();
        storage->add(entity, component);
    }

    template <typename T>
    void removeComponent(EntityID entity) {
        static_assert(std::is_base_of<Component, T>::value, "T must inherit from Component");
        auto it = storages.find(std::type_index(typeid(T)));
        if (it != storages.end()) {
            it->second->removeEntity(entity);
        }
    }

    template <typename T>
    bool hasComponent(EntityID entity) const {
        auto storage = getStorageConst<T>();
        return storage != nullptr && storage->hasEntity(entity);
    }

    template <typename T>
    T& getComponent(EntityID entity) {
        auto storage = getStorage<T>();
        return storage->get(entity);
    }

    template <typename T>
    const T& getComponent(EntityID entity) const {
        auto storage = getStorageConst<T>();
        if (!storage) throw std::out_of_range("Component type not found in registry");
        return storage->get(entity);
    }

    // View for two components - returns iterator-like object
    template <typename T1, typename T2>
    class View2 {
    private:
        const SparseSet<T1>* s1;
        const SparseSet<T2>* s2;

    public:
        View2(const SparseSet<T1>* ss1, const SparseSet<T2>* ss2) : s1(ss1), s2(ss2) {}

        class Iterator {
        private:
            const std::vector<EntityID>* dense;
            const SparseSet<T2>* s2;
            size_t index;

        public:
            Iterator(const std::vector<EntityID>* d, const SparseSet<T2>* ss2, size_t idx)
                : dense(d), s2(ss2), index(idx) {
                // Skip entities that don't have T2
                while (index < dense->size() && !s2->hasEntity((*dense)[index])) {
                    ++index;
                }
            }

            bool operator!=(const Iterator& other) const {
                return index != other.index;
            }

            EntityID operator*() const {
                return (*dense)[index];
            }

            Iterator& operator++() {
                ++index;
                while (index < dense->size() && !s2->hasEntity((*dense)[index])) {
                    ++index;
                }
                return *this;
            }
        };

        Iterator begin() const {
            if (!s1 || s1->getDense().empty()) {
                return Iterator(&s1->getDense(), s2, s1->getDense().size());
            }
            return Iterator(&s1->getDense(), s2, 0);
        }

        Iterator end() const {
            if (!s1) return Iterator(&s1->getDense(), s2, 0);
            return Iterator(&s1->getDense(), s2, s1->getDense().size());
        }
    };

    template <typename T1, typename T2>
    View2<T1, T2> view() {
        return View2<T1, T2>(getStorageConst<T1>(), getStorageConst<T2>());
    }
};
