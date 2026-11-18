add_library(energy_clientlib STATIC src/EnergyClient.cpp)
target_include_directories(energy_clientlib PUBLIC include)
target_link_libraries(energy_clientlib PUBLIC energy_core PRIVATE energy_net)

if(MSVC)
  target_compile_options(energy_clientlib PRIVATE /W4 /permissive-)
endif()
