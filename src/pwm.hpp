
#pragma once

#include <chrono>
#include <expected>

#include "error.hpp"

using namespace std;
using namespace chrono;

namespace rfs {

class Pwm {

public:

    virtual expected<void, rfs::Error> set_frequency(float frequency) = 0;
    virtual expected<void, rfs::Error> set_duty_cycle(float duty_cycle) = 0;

};

}