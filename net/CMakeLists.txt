add_library(energy_net STATIC src/Net.cpp)
target_include_directories(energy_net PUBLIC include)
target_link_libraries(energy_net PUBLIC energy_core ws2_32)

if(MSVC)
  target_compile_options(energy_net PRIVATE /W4 /permissive-)
endif()
