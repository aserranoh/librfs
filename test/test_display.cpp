
#include "i2c.hpp"
#include "i2cdisplay.hpp"
#include "face.hpp"

#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    rfs::i2c i;
    const bool result = i.open("/dev/i2c-1", 0x27);
    if (!result) {
        const int errnum = errno;
        std::cout << strerror(errnum) << std::endl;
        return 1;
    }

    rfs::i2cdisplay<rfs::i2c> d(16, 2, i);
    d.init();
    d.set_backlight_on();
    d.clear();

    rfs::face f(d);
    f.init();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    while (1) {
        f.blink();
        std::this_thread::sleep_for(std::chrono::seconds(3));

        f.look_at(rfs::eye_side::right, rfs::eye_direction::right);
        f.look_at(rfs::eye_side::left, rfs::eye_direction::right);
        std::this_thread::sleep_for(std::chrono::seconds(2));

        f.look_at(rfs::eye_side::right, rfs::eye_direction::left);
        f.look_at(rfs::eye_side::left, rfs::eye_direction::left);
        std::this_thread::sleep_for(std::chrono::seconds(2));

        f.look_at(rfs::eye_side::right, rfs::eye_direction::down_left);
        f.look_at(rfs::eye_side::left, rfs::eye_direction::down_right);
    }
    

}