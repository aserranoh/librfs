
#include <cassert>
#include <iostream>

#include "../src/servo.hpp"
#include "../src/pca9685.hpp"

using namespace rfs;
using namespace std;

#define PCA9685_DEVICE "/dev/i2c-1"
#define PCA9685_ADDRESS 0x40

void test_open() {
    Pca9685 p;

    // Open the device (happy path)
    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);
    p.close();

    // Open a wrong device (unhappy path)
    auto res_wrong_device = p.open("/dev/wrong_device", PCA9685_ADDRESS);
    assert(!res_wrong_device);
    assert(res_wrong_device.error().name() == "ENOENT");

    // Open a device with a wrong I2C address (unhappy path)
    auto res_wrong_address = p.open(PCA9685_DEVICE, 0x20);
    assert(!res_wrong_address);
    assert(res_wrong_address.error().name() == "EREMOTEIO");
}

void test_close_not_opened() {
    Pca9685 p;

    // Close the device, it was not opened (unhappy path)
    auto res_close = p.close();
    assert(!res_close);
    assert(res_close.error().name() == "EBADF");
}

void test_sleep() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);
    auto res_sleep = p.sleep();
    assert(res_sleep);

    // Check that the device is asleep
    auto res_asleep = p.asleep();
    assert(res_asleep);
    assert(*res_asleep);

    // Check that the device doesn't need a restart
    auto res_needs_restart = p.needs_restart();
    assert(res_needs_restart);
    assert(!(*res_needs_restart));

    // Restart, as it didn't need it, it returns that the device wasn't restarted
    auto res_restart = p.restart();
    assert(res_restart);
    assert(!(*res_restart));
    res_asleep = p.asleep();
    assert(res_asleep);
    assert(!(*res_asleep));

    // Activate a channel and put the device to sleep
    auto res_set_times = p.set_on_off_times(0, 0.25, 0.5);
    assert(res_set_times);
    res_sleep = p.sleep();
    assert(res_sleep);
    res_asleep = p.asleep();
    assert(res_asleep);
    assert(*res_asleep);
    res_needs_restart = p.needs_restart();
    assert(res_needs_restart);
    assert(*res_needs_restart);
    res_restart = p.restart();
    assert(res_restart);
    assert(*res_restart);
}

void test_subaddress1() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_sub1 = p.set_subaddress1(0xea);
    assert(res_set_sub1);
    auto res_set_sub1_enabled = p.set_subaddress1_enabled(true);
    assert(res_set_sub1_enabled);
    auto res_get_sub1 = p.subaddress1();
    assert(res_get_sub1);
    assert(*res_get_sub1 == 0xea);
    auto res_get_sub1_enabled = p.subaddress1_enabled();
    assert(res_get_sub1_enabled);
    assert(*res_get_sub1_enabled);
    p.close();

    Pca9685 p2;
    res = p2.open(PCA9685_DEVICE, 0x75);
    assert(res);
    res_get_sub1 = p2.subaddress1();
    assert(res_get_sub1);
    assert(*res_get_sub1 == 0xea);
    p2.close();

    res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);
    res_set_sub1_enabled = p.set_subaddress1_enabled(false);
    assert(res_set_sub1_enabled);
}

void test_subaddress2() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_sub2 = p.set_subaddress2(0xec);
    assert(res_set_sub2);
    auto res_set_sub2_enabled = p.set_subaddress2_enabled(true);
    assert(res_set_sub2_enabled);
    auto res_get_sub2 = p.subaddress2();
    assert(res_get_sub2);
    assert(*res_get_sub2 == 0xec);
    auto res_get_sub2_enabled = p.subaddress2_enabled();
    assert(res_get_sub2_enabled);
    assert(*res_get_sub2_enabled);
    p.close();

    Pca9685 p2;
    res = p2.open(PCA9685_DEVICE, 0x76);
    assert(res);
    res_get_sub2 = p2.subaddress2();
    assert(res_get_sub2);
    assert(*res_get_sub2 == 0xec);
    p2.close();

    res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);
    res_set_sub2_enabled = p.set_subaddress2_enabled(false);
    assert(res_set_sub2_enabled);
}

void test_subaddress3() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_sub3 = p.set_subaddress3(0xee);
    assert(res_set_sub3);
    auto res_set_sub3_enabled = p.set_subaddress3_enabled(true);
    assert(res_set_sub3_enabled);
    auto res_get_sub3 = p.subaddress3();
    assert(res_get_sub3);
    assert(*res_get_sub3 == 0xee);
    auto res_get_sub3_enabled = p.subaddress3_enabled();
    assert(res_get_sub3_enabled);
    assert(*res_get_sub3_enabled);
    p.close();

    Pca9685 p2;
    res = p2.open(PCA9685_DEVICE, 0x77);
    assert(res);
    res_get_sub3 = p2.subaddress3();
    assert(res_get_sub3);
    assert(*res_get_sub3 == 0xee);
    p2.close();

    res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);
    res_set_sub3_enabled = p.set_subaddress3_enabled(false);
    assert(res_set_sub3_enabled);
}

void test_all_call_address() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_ac = p.set_all_call_address(0xe6);
    assert(res_set_ac);
    auto res_set_ac_enabled = p.set_all_call_address_enabled(true);
    assert(res_set_ac_enabled);
    auto res_get_ac = p.all_call_address();
    assert(res_get_ac);
    assert(*res_get_ac == 0xe6);
    auto res_get_ac_enabled = p.all_call_address_enabled();
    assert(res_get_ac_enabled);
    assert(*res_get_ac_enabled);
    p.close();

    Pca9685 p2;
    res = p2.open(PCA9685_DEVICE, 0x73);
    assert(res);
    res_get_ac = p2.all_call_address();
    assert(res_get_ac);
    assert(*res_get_ac == 0xe6);
    p2.close();

    res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);
    res_set_ac_enabled = p.set_all_call_address_enabled(false);
    assert(res_set_ac_enabled);
}

void test_invert() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_inverted = p.set_output_inverted(true);
    assert(res_set_inverted);
    auto res_get_inverted = p.output_inverted();
    assert(res_get_inverted);
    assert(*res_get_inverted);

    res_set_inverted = p.set_output_inverted(false);
    assert(res_set_inverted);
    res_get_inverted = p.output_inverted();
    assert(res_get_inverted);
    assert(!(*res_get_inverted));
}

void test_output_change() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_oc = p.set_output_change(Pca9685OutputChange::OnAck);
    assert(res_set_oc);
    auto res_get_oc = p.output_change();
    assert(res_get_oc);
    assert(*res_get_oc == Pca9685OutputChange::OnAck);

    res_set_oc = p.set_output_change(Pca9685OutputChange::OnStop);
    assert(res_set_oc);
    res_get_oc = p.output_change();
    assert(res_get_oc);
    assert(*res_get_oc == Pca9685OutputChange::OnStop);
}

void test_external_driver() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_ext = p.set_external_driver(true);
    assert(res_set_ext);
    auto res_get_ext = p.external_driver();
    assert(res_get_ext);
    assert(*res_get_ext);

    res_set_ext = p.set_external_driver(false);
    assert(res_set_ext);
    res_get_ext = p.external_driver();
    assert(res_get_ext);
    assert(!(*res_get_ext));
}

void test_output_disabled_mode() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_oc = p.set_output_disabled_mode(Pca9685OutputDisabledMode::HighImpedance);
    assert(res_set_oc);
    auto res_get_oc = p.output_disabled_mode();
    assert(res_get_oc);
    assert(*res_get_oc == Pca9685OutputDisabledMode::HighImpedance);

    res_set_oc = p.set_output_disabled_mode(Pca9685OutputDisabledMode::Low);
    assert(res_set_oc);
    res_get_oc = p.output_disabled_mode();
    assert(res_get_oc);
    assert(*res_get_oc == Pca9685OutputDisabledMode::Low);
}

void test_frequency() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    // Set frequency, wrong frequency
    auto res_set_freq = p.set_frequency(0.0);
    assert(!res_set_freq);
    assert(res_set_freq.error().name() == "EINVAL");

    // Set frequency, wrong clock frequency
    res_set_freq = p.set_frequency(50, -1.0);
    assert(!res_set_freq);
    assert(res_set_freq.error().name() == "EINVAL");

    // Set frequency, too big
    res_set_freq = p.set_frequency(2000);
    assert(!res_set_freq);
    assert(res_set_freq.error().name() == "EINVAL");

    // Set frequency, too small
    res_set_freq = p.set_frequency(23);
    assert(!res_set_freq);
    assert(res_set_freq.error().name() == "EINVAL");

    res_set_freq = p.set_frequency(50);
    assert(res_set_freq);

    auto res_get_freq = p.frequency(-1.0);
    assert(!res_get_freq);
    assert(res_get_freq.error().name() == "EINVAL");

    res_get_freq = p.frequency();
    assert(res_get_freq);
    assert(lround(*res_get_freq) == 50);
}

void test_on_off_times() {
    Pca9685 p;

    auto res = p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    assert(res);

    auto res_set_times = p.set_on_off_times(16, 0.5, 0.75);
    assert(!res_set_times);
    assert(res_set_times.error().name() == "EINVAL");

    res_set_times = p.set_on_off_times(0, -1.0, 0.75);
    assert(!res_set_times);
    assert(res_set_times.error().name() == "EINVAL");

    res_set_times = p.set_on_off_times(0, 0.5, -1.0);
    assert(!res_set_times);
    assert(res_set_times.error().name() == "EINVAL");

    auto res_get_times = p.on_off_times(16);
    assert(!res_get_times);
    assert(res_get_times.error().name() == "EINVAL");

    for (int channel = 0; channel < 16; channel++) {
        res_set_times = p.set_on_off_times(channel, 0.5, 0.75);
        assert(res_set_times);
        this_thread::sleep_for(chrono::milliseconds(40));

        res_get_times = p.on_off_times(channel);
        assert(res_get_times);
        assert(res_get_times->on == 0.5);
        assert(res_get_times->off == 0.75);
    }

    res_set_times = p.set_on_off_times(Pca9685::ALL_CHANNELS, 0.25, 0.85);
    assert(res_set_times);
    this_thread::sleep_for(chrono::milliseconds(40));

    res_get_times = p.on_off_times(1);
    assert(res_get_times);
    assert(abs(res_get_times->on - 0.25) < 0.001);
    assert(abs(res_get_times->off - 0.85) < 0.001);
}

void test_servo() {
    Pca9685 p;

    p.open(PCA9685_DEVICE, PCA9685_ADDRESS);
    p.restart();
    p.set_frequency(50.0);

    unique_ptr<Pwm> ch0 = *(p.pwm(0));
    Servo s0(ch0, 0.05);
    s0.set_angle(-90.0);
    this_thread::sleep_for(chrono::seconds(1));
    s0.set_angle(0.0);
    this_thread::sleep_for(chrono::seconds(1));
    s0.set_angle(90.0);
    this_thread::sleep_for(chrono::seconds(1));

    unique_ptr<Pwm> ch1 = *(p.pwm(1));
    Servo s1(ch1, 0.05);
    s1.set_angle(-90.0);
    this_thread::sleep_for(chrono::seconds(1));
    s1.set_angle(0.0);
    this_thread::sleep_for(chrono::seconds(1));
    s1.set_angle(90.0);
    this_thread::sleep_for(chrono::seconds(1));

    unique_ptr<Pwm> ch2 = *(p.pwm(2));
    Servo s2(ch2, 0.05);
    s2.set_angle(-90.0);
    this_thread::sleep_for(chrono::seconds(1));
    s2.set_angle(0.0);
    this_thread::sleep_for(chrono::seconds(1));
    s2.set_angle(90.0);
    this_thread::sleep_for(chrono::seconds(1));

    unique_ptr<Pwm> ch3 = *(p.pwm(3));
    Servo s3(ch3, 0.05);
    s3.set_angle(-90.0);
    this_thread::sleep_for(chrono::seconds(1));
    s3.set_angle(0.0);
    this_thread::sleep_for(chrono::seconds(1));
    s3.set_angle(90.0);
    this_thread::sleep_for(chrono::seconds(1));
}

int main() {
    /*test_open();
    test_close_not_opened();
    test_sleep();
    test_subaddress1();
    test_subaddress2();
    test_subaddress3();
    //test_all_call_address();
    test_invert();
    test_output_change();
    test_external_driver();
    test_output_disabled_mode();
    test_frequency();
    test_on_off_times();*/

    test_servo();
}