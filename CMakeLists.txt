cmake_minimum_required(VERSION 3.28.3)
project(cf)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx512f")

add_subdirectory(spdlog)

file(GLOB Sources CONFIGURE_DEPENDS "*.cpp")

foreach(Source ${Sources})
  get_filename_component(Exec ${Source} NAME_WE)
  add_executable(${Exec} ${Source})
  target_link_libraries(${Exec} PUBLIC pthread spdlog::spdlog)
endforeach()