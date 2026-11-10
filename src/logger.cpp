#include "logger.hpp"

// Returns a logger, useful for printing debug messages
Paper::ConstLoggerContext<15UL> getLogger()
{
    static constexpr auto fastContext = Paper::ConstLoggerContext("RecentlyPlayed");
    return fastContext;
}