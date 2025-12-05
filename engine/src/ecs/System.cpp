#include "System.h"
#include "../core/Log.h"
#include "Entity.h" 

#include <algorithm> // For std::find and std::erase

namespace Engine {

    /**
     * @brief Adds an entity to the system's internal list.
     * @details This function checks if the entity is already present before adding it.
     */
    void System::addEntity(Entity* entity) {
        if (!entity) {
            Log::warn("Attempted to add a null entity to a system.");
            return;
        }

        auto it = std::find(m_Entities.begin(), m_Entities.end(), entity);
        
        if (it == m_Entities.end()) {
            // Entity not found, so add it
            m_Entities.push_back(entity);
            Log::info("System added entity: " + entity->getName()); 
        } else {
            Log::warn("Entity " + entity->getName() + " already subscribed to this system.");
        }
    }

    /**
     * @brief Removes an entity from the system's internal list.
     * @details This uses std::remove and erase idiom to remove the pointer from the vector.
     */
    void System::removeEntity(Entity* entity) {
        if (!entity) return;

        // Use erase-remove idiom to efficiently remove the entity pointer (using m_Entities)
        m_Entities.erase(
            std::remove(m_Entities.begin(), m_Entities.end(), entity),
            m_Entities.end()
        );
    }

    // Since the system is not pure virtual, we need to ensure all virtual methods 
    // declared in the header have definitions, even if they are empty.

} // namespace Engine