file(GLOB TestSources CONFIGURE_DEPENDS "*.cpp")

foreach(Source ${TestSources})
    get_filename_component(Exec ${Source} NAME_WE)
    add_executable(${Exec} ${Source})
    target_include_directories(${Exec} PUBLIC ${CMAKE_SOURCE_DIR}/kit/include ${CMAKE_SOURCE_DIR}/googletest/googletest/include)
    target_link_libraries(${Exec} PUBLIC gtest_main)
endforeach()