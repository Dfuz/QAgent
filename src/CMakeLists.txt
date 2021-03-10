set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(QAgent
    main.cpp
    ${SOURCES}
    ${HEADERS})

target_link_libraries(QAgent
    ${QT_MODULES})
