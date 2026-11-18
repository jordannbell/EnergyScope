add_library(energy_core STATIC
  src/MeasurementRepository.cpp
  src/Protocol.cpp
  src/ChartScale.cpp
)
target_include_directories(energy_core PUBLIC include)
target_link_libraries(energy_core PUBLIC sqlite3)

if(MSVC)
  target_compile_options(energy_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(energy_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
