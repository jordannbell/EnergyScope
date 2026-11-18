#pragma once

#include <string>

#include "energy/MeasurementRepository.hpp"

namespace energy {

// Text protocol, one request per line (the line has no trailing '\n').
//
//   ADD <site> <timestamp> <powerKw>   ->  "OK\n"
//   GET <site>                         ->  "OK <count>\n" then <count> lines
//                                          "<timestamp> <powerKw>\n"
//   anything else / bad values         ->  "ERR <reason>\n"
//
// <site> may not contain spaces. The function never throws: database
// failures are reported as "ERR ...". It does no network I/O, so it can be
// unit-tested without sockets.
std::string handleRequest(MeasurementRepository& repo, const std::string& line);

}  // namespace energy
