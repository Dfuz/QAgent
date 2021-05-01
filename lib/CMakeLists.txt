cmake_minimum_required(VERSION 3.14)

SET(BUILD_SHARED_LIBS ON)
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)

list(APPEND MONITOR_LIB_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/linux_memoryload.cpp
	${CMAKE_CURRENT_LIST_DIR}/linux_networkload.cpp
	${CMAKE_CURRENT_LIST_DIR}/linux_systemutil.cpp
	${CMAKE_CURRENT_LIST_DIR}/linux_cpuload.cpp
	${CMAKE_CURRENT_LIST_DIR}/util/timer.cpp)

list(APPEND MONITOR_LIB_HEADERS
	${CMAKE_CURRENT_LIST_DIR}/linux_memoryload.hpp
	${CMAKE_CURRENT_LIST_DIR}/linux_networkload.hpp
	${CMAKE_CURRENT_LIST_DIR}/linux_systemutil.hpp
	${CMAKE_CURRENT_LIST_DIR}/linux_cpuload.hpp
	${CMAKE_CURRENT_LIST_DIR}/util/timer.hpp
	${CMAKE_CURRENT_LIST_DIR}/util/record_value.hpp)


add_library(linuxmonitoring STATIC
	    ${MONITOR_LIB_SOURCES}
	    ${MONITOR_LIB_HEADERS})
target_compile_options(linuxmonitoring PRIVATE ${COMPILER_SETTINGS})

target_link_libraries(linuxmonitoring
        pthread
        rt
        )
