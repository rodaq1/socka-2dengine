#pragma once

namespace Engine {
    class Application {
    public:
        virtual ~Application() = default;

        virtual void run() = 0;
    };
}