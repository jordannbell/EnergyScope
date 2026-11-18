add_executable(core_tests
  MeasurementRepositoryTest.cpp
  ProtocolTest.cpp
  ChartScaleTest.cpp
  GeometryTest.cpp
  FormatTest.cpp
  PdfChartTest.cpp
  FileIoTest.cpp
  LegacyReportTest.cpp
  ReportBuilderTest.cpp
)
target_link_libraries(core_tests PRIVATE energy_core energy_legacy GTest::gtest_main)

if(WIN32)
  target_sources(core_tests PRIVATE EnergyClientTest.cpp)
  target_link_libraries(core_tests PRIVATE energy_clientlib energy_net)
endif()

include(GoogleTest)
# Discovery runs the fresh executable once at build time. The default limit
# of 5 s failed once on a clean Release build (first launch of a new .exe is
# slow, e.g. antivirus scan), so give it more room.
gtest_discover_tests(core_tests DISCOVERY_TIMEOUT 30)
