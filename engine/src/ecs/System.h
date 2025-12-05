#pragma once
#include <set>
#include <vector>
#include <typeinfo>

namespace Engine {
    class Entity;
}

namespace Engine {
    /**
     * @brief Zakladna class pre vsetky ECS systemy.
     * * Systemy definuju logiku a operuju na entitach ktore maju specificky set komponentov
     */
    class System {
    public:
        virtual ~System() = default;

        virtual void onInit() {};
        virtual void onUpdate(float dt) {};
        virtual void onShutdown() {};
        virtual void addEntity(Entity* entity);
        virtual void removeEntity(Entity* entity);
        const std::vector<Entity*>& getSystemEntities() const {
            return m_Entities;
        }
        const std::set<const std::type_info*>& getComponentSignature() const {
            return m_ComponentSignature;
        }
    protected:
        /**
         * @brief deklaruje component requirement pre dany system
         * * Systemy toto volaju v svojich constructoroch aby bolo zadefinovane ake komponenty procesuju.
         * @tparam TComponent component type required.
         */
        template <typename TComponent>
        void requireComponent() {
            m_ComponentSignature.insert(&typeid(TComponent));
        }
    
    private:
        std::vector<Entity*> m_Entities;
        std::set<const std::type_info*> m_ComponentSignature;
    };
}