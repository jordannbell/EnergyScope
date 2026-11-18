cmake_minimum_required(VERSION 3.20)
project(EnergyScope VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# SQLite: single-file "amalgamation", compiled as a small static library.
FetchContent_Declare(
  sqlite_amalgamation
  URL https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip
)
FetchContent_MakeAvailable(sqlite_amalgamation)
add_library(sqlite3 STATIC ${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite_amalgamation_SOURCE_DIR})

# GoogleTest for unit tests.
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(core)
if(WIN32)
  add_subdirectory(net)
  add_subdirectory(server)
endif()

enable_testing()
add_subdirectory(tests)
