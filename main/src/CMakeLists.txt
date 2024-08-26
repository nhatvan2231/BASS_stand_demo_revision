add_executable(demo_main main.cpp main.h)
#add_executable(demo_detection detection.cpp detection.h)


target_link_libraries(demo_main PRIVATE demo_lib)
#target_link_libraries(demo_detection PRIVATE demo_lib)
