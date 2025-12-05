#pragma once

#include <string_view>
#include <iostream>

namespace Engine {
    /**
     * @brief static logging system enginu.
     * staticke metody logovania, callable anywhere in the engine
     * wrapper around std::cout/std::cerr
     */

    class Log {
    private:
        Log() = default;

        static void logMessage(std::string_view prefix, std::string_view message, std::ostream& os);
    
    public:
        static void info(std::string_view msg);
        static void warn(std::string_view msg);
        static void error(std::string_view msg);
    };

}
