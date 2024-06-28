
#include <expected>
#include <memory>

#include "error.hpp"
#include "pwm.hpp"

using namespace std;
using namespace chrono;

namespace rfs {

class Servo {

public:

    static constexpr float SERVO_MIN_ANGLE = -90.0;
    static constexpr float SERVO_MAX_ANGLE = 90.0;

    Servo(unique_ptr<Pwm> &driver, float half_angle_duty_cycle = SERVO_HALF_ANGLE_DUTY_CYCLE_DEFAULT, float offset = SERVO_OFFSET_DEFAULT):
        driver(move(driver)), half_angle_duty_cycle(half_angle_duty_cycle), offset(offset)
    {}

    ~Servo() {}

    expected<void, Error> init() {
        return driver->set_frequency(SERVO_FREQUENCY);
    }

    expected<void, Error> set_angle(float angle) {
        if (angle < SERVO_MIN_ANGLE || angle > SERVO_MAX_ANGLE) {
            return unexpected(EINVAL);
        }
        return driver->set_duty_cycle(angle / SERVO_MAX_ANGLE * half_angle_duty_cycle + offset);
    }

private:

    static constexpr float SERVO_FREQUENCY = 50.0;
    static constexpr float SERVO_HALF_ANGLE_DUTY_CYCLE_DEFAULT = 0.025;
    static constexpr float SERVO_OFFSET_DEFAULT = 0.075;

    unique_ptr<Pwm> driver;
    float half_angle_duty_cycle;
    float offset;

};

}