set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Test)

if(Qt5Test_FOUND)

	# enable_testing(true)

	# include_directories(${TOP_DIR}/src)

	# add_executable(qagent_test
	#     qagent.cpp
	#     ${SOURCES}
	#     ${HEADERS})

	# add_test(NAME qagent_test COMMAND qagent_test)
	# target_link_libraries(qagent_test PRIVATE Qt5::Test ${QT_MODULES})

endif()
