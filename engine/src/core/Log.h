#pragma once
#include <string_view>
#include <iostream>
#include <functional>

namespace Engine {
    /**
     * @brief Static logging system for the engine.
     * * This class handles internal engine logging. It supports a callback
     * mechanism so the Editor can "hook" into engine logs without the 
     * engine needing to know about Editor-specific classes (like Console).
     */
    class Log {
    public:
        // Level enum for structured callbacks
        enum class Level { Info, Warn, Error };

        // Define a callback type: (Message, Level)
        using LogCallback = std::function<void(std::string_view, Level)>;

    private:
        Log() = default;

        // The callback instance - allows the Editor to listen to Engine logs
        static LogCallback s_Callback;

        /**
         * @brief Internal helper to route messages to both stdout and the callback.
         */
        static void logMessage(std::string_view prefix, std::string_view message, Level level, std::ostream& os);
    
    public:
        /**
         * @brief Sets the function to be called whenever a log occurs.
         * Used by the Editor to redirect logs to its UI console.
         */
        static void SetCallback(LogCallback cb) { s_Callback = cb; }

        static void info(std::string_view msg);
        static void warn(std::string_view msg);
        static void error(std::string_view msg);
    };

}