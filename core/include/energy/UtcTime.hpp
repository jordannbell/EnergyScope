#pragma once

#include <cstdint>
#include <string>

namespace energy {

// Unix time -> "DD/MM HH:MM" in UTC. Independent of the PC time zone and of
// any OS API, so it gives the same text everywhere (and is easy to test).
std::string formatUtcDayMinute(std::int64_t unixSeconds);

}  // namespace energy
