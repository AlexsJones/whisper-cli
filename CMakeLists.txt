cmake_minimum_required(VERSION 2.8)
set(INSTALL_PREFIX /usr)
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
project(whisper-cli)
option(RUN_TESTS "Tests" OFF)
execute_process(COMMAND git submodule init)
execute_process(COMMAND git submodule update)
execute_process(COMMAND cmake . WORKING_DIRECTORY whisper-core)
execute_process(COMMAND make . WORKING_DIRECTORY whisper-core)
execute_process(COMMAND sudo make install . WORKING_DIRECTORY whisper-core)

if(RUN_TESTS)
  execute_process(COMMAND ./test/run_tests)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS "-Wno-pointer-sign")
endif()

file(GLOB SRC "src/**/*.c")
include_directories(src)
include_directories("whisper-core/whisper-core")

if(NOT DEBUG)
  add_executable(whisper-cli src/main.c ${SRC})
  target_link_libraries(whisper-cli -ljnxc -lcrypto -lssl -g -lprotobuf-c
    -lncurses -lpthread -lwhisper-core)
else()
  add_definitions("-g")
  file(GLOB CORE_SRC "whisper-core/src/**/*.c")
  add_executable(whisper-cli src/main.c ${SRC} ${CORE_SRC})
  target_link_libraries(whisper-cli -ljnxc -lcrypto -lssl -g -lprotobuf-c
    -lncurses -lpthread)
endif()

install(TARGETS whisper-cli DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
