file(GLOB test_files PRIVATE ./*.cpp)

foreach(file IN LISTS test_files)
    get_filename_component(basename ${file} NAME_WE)
    add_executable(${basename} ${file})
    target_link_libraries(${basename} ${DEPEND_LIBS})
    add_test(NAME ${basename} COMMAND ${basename})
endforeach()

