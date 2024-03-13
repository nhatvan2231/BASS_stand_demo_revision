add_executable(drone_demo mavlink_control.cpp mavlink_control.h)
add_executable(detection detection.cpp detection.h)


target_link_libraries(drone_demo PRIVATE demo_lib)
target_link_libraries(detection PRIVATE demo_lib)
