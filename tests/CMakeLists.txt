add_executable(core_tests MeasurementRepositoryTest.cpp ProtocolTest.cpp)
target_link_libraries(core_tests PRIVATE energy_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(core_tests)
