add_executable(energy_server main.cpp)
target_link_libraries(energy_server PRIVATE energy_net)

if(MSVC)
  target_compile_options(energy_server PRIVATE /W4 /permissive-)
endif()
