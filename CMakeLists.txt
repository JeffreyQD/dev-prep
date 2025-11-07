cmake_minimum_required(VERSION 3.28.3)
project(cf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f -O3 -pthread")

add_subdirectory(spdlog)
add_subdirectory(googletest)
add_subdirectory(benchmark)

add_subdirectory(tests)
add_subdirectory(profiles)

file(GLOB Sources CONFIGURE_DEPENDS "*.cpp")

foreach(Source ${Sources})
  get_filename_component(Exec ${Source} NAME_WE)
  add_executable(${Exec} ${Source})
  target_include_directories(${Exec} PUBLIC ${CMAKE_SOURCE_DIR}/kit/include ${CMAKE_SOURCE_DIR}/benchmark/include)
  target_link_libraries(${Exec} PUBLIC spdlog::spdlog benchmark::benchmark)
endforeach()