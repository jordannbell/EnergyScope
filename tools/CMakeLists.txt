add_executable(energy_export export_pdf.cpp)
target_link_libraries(energy_export PRIVATE energy_clientlib)

if(MSVC)
  target_compile_options(energy_export PRIVATE /W4 /permissive-)
endif()
