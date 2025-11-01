file(GLOB ProfileSources CONFIGURE_DEPENDS "*.cpp")

foreach(Source ${ProfileSources})
  get_filename_component(Exec ${Source} NAME_WE)
  add_executable(${Exec} ${Source})
  target_include_directories(${Exec} PUBLIC ${CMAKE_SOURCE_DIR}/kit/include ${CMAKE_SOURCE_DIR}/benchmark/include)
  target_link_libraries(${Exec} PUBLIC pthread benchmark::benchmark)
endforeach()