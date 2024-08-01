
add_executable(test_pca9685 test_pca9685.cpp)
target_link_libraries(test_pca9685 i2c)

add_executable(test_webserver test_webserver.cpp mongoose.c)

add_executable(test_kinematic_chain test_kinematic_chain.cpp)