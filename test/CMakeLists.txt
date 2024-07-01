
add_executable(test_pca9685 test_pca9685.cpp)
target_link_libraries(test_pca9685 i2c)

add_executable(test_webserver test_webserver.cpp mongoose.c)