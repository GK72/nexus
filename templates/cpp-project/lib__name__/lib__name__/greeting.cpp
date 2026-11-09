/**
 * Part of @part_of@
 *
 * @brief   A placeholder module from the project template: replace it with the first real one.
 *
 * @author      @author@
 * @created     @date@
 * @updated     @date@
 * @status      experimental
 * @reviewed    none
 */

#include <lib@name@/greeting.hpp>

#include <format>

namespace @namespace@ {

auto greet(std::string_view who) -> std::string {
    return std::format("Hello, {}!", who.empty() ? "world" : who);
}

} // namespace @namespace@
