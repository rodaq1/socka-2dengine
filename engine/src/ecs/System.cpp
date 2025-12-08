#include "System.h"
#include "../core/Log.h"
#include "Entity.h" 

#include <algorithm> 

namespace Engine {

    /**
     * @brief Pridanie entity do listu entit systemu.
     * @details Kontrola ci je entita uz present pred pridanim.
     */
    void System::addEntity(Entity* entity) {
        if (!entity) {
            Log::warn("Attempted to add a null entity to a system.");
            return;
        }

        auto it = std::find(m_Entities.begin(), m_Entities.end(), entity);
        
        if (it == m_Entities.end()) {
            m_Entities.push_back(entity);
            Log::info("System added entity: " + entity->getName()); 
        } else {
            Log::warn("Entity " + entity->getName() + " already subscribed to this system.");
        }
    }

    /**
     * @brief Odstrani entitu z listu entit systemu.
     */
    void System::removeEntity(Entity* entity) {
        if (!entity) return;

        m_Entities.erase(
            std::remove(m_Entities.begin(), m_Entities.end(), entity),
            m_Entities.end()
        );
    }


}