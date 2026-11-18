# MFC desktop client. Uses MFC as a DLL (_AFXDLL); needs the MFC component
# of Visual Studio. CMAKE_MFC_FLAG is ignored by the Ninja generator, so
# the definition is set by hand.

add_executable(energy_client WIN32
  EnergyApp.cpp
  EnergyDoc.cpp
  MainFrame.cpp
  MeasurementView.cpp
  ChartView.cpp
  SiteDialog.cpp
  TimeFormat.cpp
  EnergyScope.rc
)

target_compile_definitions(energy_client PRIVATE _UNICODE UNICODE _AFXDLL)
target_link_libraries(energy_client PRIVATE energy_clientlib opengl32)

if(MSVC)
  target_compile_options(energy_client PRIVATE /W4 /permissive- /utf-8)
  # MFC with Unicode needs the wide-character entry point.
  target_link_options(energy_client PRIVATE /ENTRY:wWinMainCRTStartup)
endif()
