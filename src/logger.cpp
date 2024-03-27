#include "logger.hpp"

// Returns a logger, useful for printing debug messages
Paper::ConstLoggerContext<15UL> getLogger()
{
    static auto fastContext = Paper::Logger::WithContext<"RecentlyPlayed">();
    return fastContext;
}