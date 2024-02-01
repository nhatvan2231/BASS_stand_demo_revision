#file(GLOB_RECURSE DRONE_FILES drone_demo/src/*.cpp)

add_library(
	demo_lib	
	#	${DRONE_FILES}

	drone_demo/src/autopilot_interface.cpp
	drone_demo/src/serial_port.cpp
	drone_demo/src/udp_port.cpp
	)

target_include_directories(demo_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/mavlink/include/mavlink/v2.0")
target_include_directories(demo_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/drone_demo/include")
