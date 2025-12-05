#pragma once
//toto je iba definicia componentu, implementacia funkcii bude robena v derived classach.


namespace Engine {

    class Entity;

    /**
     * @brief base classa pre componenty v ECS (entity-component system) tohoto enginu.
     * Componenty definuju behavior a data entit.
     */
    class Component {
    public:
        virtual ~Component() = default;

        virtual void onInit() {};
        virtual void onUpdate(float dt) {};
        virtual void onRender() {};
        virtual void onShutdown() {};

        //link back to the owner entity
        Entity* owner = nullptr;
    };
}