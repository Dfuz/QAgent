set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Test)

if(Qt5Test_FOUND)

	enable_testing(true)

	include_directories(${TOP_DIR}/src)

	add_executable(fcagent
	    fcagent.cpp
	    ${SOURCES}
	    ${HEADERS})

	add_test(NAME fcagent COMMAND fcmanager)
	target_link_libraries(fcagent PRIVATE Qt5::Test ${QT_MODULES})

endif()
