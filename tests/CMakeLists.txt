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
gtest_discover_tests(core_tests)
