/**
 * @brief Basic functionality like a logger and stuff
 */
#pragma once

namespace Atelier
{
typedef uint32_t result;
static constexpr result k_success = 0;

/**
 * @brief Stupid cursed logger. Uses cursed static elements, so I'm not sure the best way to share between dynamic
 * loadable elements
 */
struct Log {
    static void init();
    static void shutdown();
    static void unformatted(const char* const msg);
    static void info(const char* const msg, ...);
    static void warn(const char* const msg, ...);
    static void error(const char* const msg, ...);
};
}  // namespace Atelier
