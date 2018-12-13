#ifndef DISCORD_TYPES_HPP
#define DISCORD_TYPES_HPP

#include "rpp/string.hpp"

#include <cstdint>
#include <vector>

namespace discord {
    using String = rpp::String;
    // Unique id
    using Snowflake = String;
    using Snowflakes = std::vector<Snowflake>;

    enum class Status {
        online,
        idle,
        do_not_disturb,
        offline,
        unchanged,
    };

    enum class Activity_Type {
        game,
        streaming,
        listening,
    };
} // namespace discord

#endif // !DISCORD_TYPES_HPP
