# Old-style module kept on purpose (see README.md). The CRT "unsafe function"
# warnings are silenced the way a 2001 project would have done it.
add_library(energy_legacy STATIC legacy_report.cpp)
target_include_directories(energy_legacy PUBLIC .)
target_compile_definitions(energy_legacy PRIVATE _CRT_SECURE_NO_WARNINGS)
