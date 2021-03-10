set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND OS_LIB_HEADERS
    os_utils.h)

list(APPEND OS_LIB_SOURCES
    os_utils.cpp)

add_library(os_utils
    ${OS_LIB_HEADERS}
    ${OS_LIB_SOURCES})

target_link_libraries(os_utils ${QT_MODULES})

add_executable(os_test
    os_test_main.cpp)

target_link_libraries(os_test os_utils procps)
