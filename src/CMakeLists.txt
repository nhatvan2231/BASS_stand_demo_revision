add_executable(drone_demo mavlink_control.cpp mavlink_control.h)

target_link_libraries(drone_demo PRIVATE demo_lib)
