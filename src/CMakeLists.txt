set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(QAgent
    main.cpp
    ${SOURCES}
    ${HEADERS})

include_directories(${TOP_DIR}/os_read_lib)

target_link_libraries(QAgent
    ${QT_MODULES}
    os_utils)
