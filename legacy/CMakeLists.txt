add_library(energy_legacy STATIC
  legacy_report.cpp
  report_builder.cpp
)
target_include_directories(energy_legacy PUBLIC .)

if(MSVC)
  target_compile_options(energy_legacy PRIVATE /W4 /permissive-)
endif()
