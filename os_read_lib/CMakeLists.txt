set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND OS_LIB_HEADERS
    os_utils.h)

list(APPEND OS_LIB_SOURCES
    os_utils.cpp)

find_library(PROCPS procps)

add_library(os_utils SHARED
    ${OS_LIB_HEADERS}
    ${OS_LIB_SOURCES})

target_link_libraries(os_utils ${QT_MODULES} ${PROCPS})
