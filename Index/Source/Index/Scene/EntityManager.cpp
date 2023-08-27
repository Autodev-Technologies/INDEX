#include "Precompiled.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Maths/Random.h"

namespace Index
{
    Entity EntityManager::Create()
    {
        INDEX_PROFILE_FUNCTION();
        auto e = m_Registry.create();
        m_Registry.emplace<IDComponent>(e);
        return Entity(e, m_Scene);
    }

    Entity EntityManager::Create(const std::string& name)
    {
        INDEX_PROFILE_FUNCTION();
        auto e = m_Registry.create();
        m_Registry.emplace<NameComponent>(e, name);
        m_Registry.emplace<IDComponent>(e);
        return Entity(e, m_Scene);
    }

    void EntityManager::Clear()
    {
        INDEX_PROFILE_FUNCTION();
        m_Registry.each([&](auto entity)
                        { m_Registry.destroy(entity); });

        m_Registry.clear();
    }

    Entity EntityManager::GetEntityByUUID(uint64_t id)
    {
        INDEX_PROFILE_FUNCTION();

        auto view = m_Registry.view<IDComponent>();
        for(auto entity : view)
        {
            auto& idComponent = m_Registry.get<IDComponent>(entity);
            if(idComponent.ID == id)
                return Entity(entity, m_Scene);
        }

        INDEX_LOG_WARN("Entity not found by ID");
        return Entity {};
    }
}
