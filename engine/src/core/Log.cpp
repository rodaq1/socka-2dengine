#include "Log.h"
#include <ctime>
#include <iomanip>
#include <chrono>
#include <sstream>

namespace Engine {

    // Initialize the static callback to null
    Log::LogCallback Log::s_Callback = nullptr;

    void Log::info(std::string_view msg) {
        logMessage("INFO", msg, Level::Info, std::cout);
    }

    void Log::warn(std::string_view msg) {
        logMessage("WARN", msg, Level::Warn, std::cout);
    }

    void Log::error(std::string_view msg) {
        logMessage("ERROR", msg, Level::Error, std::cerr);
    }

    void Log::logMessage(std::string_view prefix, std::string_view message, Level level, std::ostream& os) {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        struct tm timeInfo;
#ifdef _WIN32
        localtime_s(&timeInfo, &time_t_now);
#else
        localtime_r(&time_t_now, &timeInfo);
#endif

        // 2. Format for standard output (Console/Terminal)
        std::stringstream ss;
        ss << "[" << std::put_time(&timeInfo, "%H:%M:%S") << "] [" << prefix << "] " << message;
        os << ss.str() << std::endl;

        // 3. Trigger the callback for the Editor UI
        if (s_Callback) {
            s_Callback(message, level);
        }
    }

}