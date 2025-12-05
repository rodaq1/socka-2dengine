#include "Log.h"

#include <ctime>
#include <iomanip>
#include <chrono>

namespace Engine {

    void Log::logMessage(std::string_view prefix, std::string_view message, std::ostream& os) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        os << "[" << std::put_time(std::localtime(&now), "%T") << "] ";
        os << "[" << prefix << "] " << message << "\n";
    }

    void Log::info(std::string_view msg) {
        logMessage("INFO", msg, std::cout);
    }

    void Log::warn(std::string_view msg) {
        logMessage("WARNING", msg, std::cout);
    }
    
    void Log::error(std::string_view msg) {
        logMessage("ERROR", msg, std::cerr);
    }
}