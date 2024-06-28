
#pragma once

extern "C"
{
    #include <fcntl.h>
    #include <i2c/smbus.h>
    #include <linux/i2c-dev.h>
    #include <sys/ioctl.h>
    #include <unistd.h>
}

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <expected>
#include <memory>
#include <thread>
#include <vector>

#include "error.hpp"
#include "pwm.hpp"

using namespace std;

namespace rfs {

/**
 * The different clocks that the **PCA9685** device can use.
 */
enum class Pca9685ClockMode {
    /**
     * The **PCA9685** uses the internal 25 MHz clock.
     */
    Internal,

    /**
     * The **PCA9685** uses an external clock signal.
     */
    External
};

/**
 * Enumerates the instants on which the **PCA9685** updates the PWM signals after a configuration change.
 */
enum class Pca9685OutputChange {
    /**
     * Update the PWM signals after the STOP condition.
     */
    OnStop,

    /**
     * Update the PWM signals after the ACK condition.
     */
    OnAck
};

/**
 * The possible states of the PWM outputs when disabled.
 */
enum class Pca9685OutputDisabledMode {
    /**
     * The PWM outputs are in LOW state.
     */
    Low             = 0,

    /**
     * The PWM outputs are HIGH if using an external driver, or HIGH_IMPEDANCE if not.
     */
    Driver          = 1,

    /**
     * The PWM outputs are in HIGH_IMPEDANCE state.
     */
    HighImpedance   = 2
};

/**
 * Contains the returned On/Off times of a PWM channel.
 */
struct Pca9685OnOffTimes {

    /**
     * Position within the cycle of the rising edge of the PWM signal, between 0.0 and 1.0
     * (0.0 corresponds to the beginning of the cycle and 1.0 to the end)
     */
    float on;

    /**
     * Position within the cycle of the falling edge of the PWM signal, between 0.0 and 1.0
     * (0.0 corresponds to the beginning of the cycle and 1.0 to the end)
     */
    float off;

    /**
     * Whether the PWM signal of this channel is always On.
     */
    bool always_on;

    /**
     * Whether the PWM signal of this channel is always Off.
     */
    bool always_off;
};

class Pca9685PwmInterface {

public:

    virtual expected<void, rfs::Error> set_on_off_times(uint32_t channel, float on_time, float off_time) = 0;

};

/**
 * This class represents a single PWM channel in the **PCA9685** device.
 */
class Pca9685Pwm: public Pwm {

public:

    Pca9685Pwm(uint32_t channel, weak_ptr<Pca9685PwmInterface> &controller):
        channel(channel), controller(controller), phase(0.0)
    {}

    virtual ~Pca9685Pwm() {}

    virtual expected<void, rfs::Error> set_duty_cycle(float duty_cycle) override {
        this->duty_cycle = duty_cycle;
        if (shared_ptr<Pca9685PwmInterface> controller_ptr = controller.lock())
            return controller_ptr->set_on_off_times(channel, phase, phase + duty_cycle);
        else
            return unexpected(rfs::Error(ENODEV));
    }

    virtual expected<void, rfs::Error> set_frequency(float frequency) override {
        return {};
    }

    virtual expected<void, rfs::Error> set_phase(float phase) {
        this->phase = phase;
        return {};
    }

private:

    weak_ptr<Pca9685PwmInterface> controller;
    uint32_t channel;
    float phase;
    float duty_cycle;

};

/**
 * This class is the main interface with a single **PCA9685** device.
 * 
 * An instance of this class is used to configure the general aspects of the **PCA9685** device.
 * Then, it can be used to instantiate objects of the class `Pca9685Pwm`, which are use
 * to control individual PWM channels.
 */
class Pca9685: public Pca9685PwmInterface {

public:

    static const uint32_t ALL_CHANNELS = 61;

    /**
     * Initializes the `Pca9685` instance.
     * 
     * Before executing any method on this instance, the method `open()` has to be called
     * to open a communication channel with a given real device.
     */
    Pca9685(): fd(-1), this_shared(this, [](auto){}) {}

    ~Pca9685() {
        set_always_off(ALL_CHANNELS, true);

        if (fd >= 0)
            close();
    }

    /**
     * Return the ALL_CALL address.
     * 
     * See `set_all_call_address()`.
     */
    expected<uint8_t, rfs::Error> all_call_address() const {
        return read_register(ALLCALL_REGISTER);
    }

    /**
     * Return whether the **PCA9685** device responds to the ALL_CALL address.
     * 
     * The ALL_CALL address is enabled at startup.
     * The ALL_CALL address can be configured using the `set_all_call_address()` method.
     */
    expected<bool, rfs::Error> all_call_address_enabled() const {
        return get_bool(MODE1_REGISTER, MODE1_ALLCALL_MASK);
    }

    /**
     * Return whether the **PCA9685** device is in sleep mode.
     */
    expected<bool, rfs::Error> asleep() const {
        return get_bool(MODE1_REGISTER, MODE1_SLEEP_MASK);
    }

    /**
     * Return the clock mode used by the **PCA9685** device.
     */
    expected<Pca9685ClockMode, rfs::Error> clock_mode() const {
        const expected<uint8_t, rfs::Error> value = read_register(MODE1_REGISTER);
        if (!value)
            return unexpected(value.error());
        return (*value & MODE1_EXTCLK_MASK) ? Pca9685ClockMode::External : Pca9685ClockMode::Internal;
    }

    /**
     * Close the communication with the **PCA9685** device.
     */
    expected<void, rfs::Error> close() {
        const expected<void, rfs::Error> set_off_result = set_always_off(ALL_CHANNELS, true);
        if (!set_off_result)
            return set_off_result;

        if (::close(fd) == -1)
            return unexpected(rfs::Error(errno));
        fd = -1;

        return {};
    }

    /**
     * Return whether the **PCA9685** is using an external driver to drive the load.
     */
    expected<bool, rfs::Error> external_driver() const {
        return get_bool(MODE2_REGISTER, MODE2_OUTDRV_MASK);
    }

    /**
     * Return the frequency of the PWM signal.
     * 
     * The PWM signal frequency depends on the **PCA9685** device's clock signal. Given that this clock
     * signal can be external, the clock frequency has to be given using the `clock_frequency` parameter.
     * By default, this parameter is set to the internal clock's frequency, which is 25MHz.
     */
    expected<float, rfs::Error> frequency(float clock_frequency = INTERNAL_CLOCK_FREQUENCY) const {
        if (clock_frequency < 0)
            return unexpected(Error(EINVAL));
        const expected<uint8_t, rfs::Error> prescale = read_register(PRESCALE_REGISTER);
        if (!prescale)
            return unexpected(prescale.error());
        return clock_frequency / ((*prescale + 1) * COUNTER_TICKS);
    }

    /**
     * Return whether the **PCA9685** device needs to be restarted.
     * 
     * The **PCA9685** needs to be restarted after it has been put to sleep without shutting down
     * all whe PWM channels. To restart it, just call the method `restart()`.
     */
    expected<bool, rfs::Error> needs_restart() const {
        return get_bool(MODE1_REGISTER, MODE1_RESTART_MASK);
    }

    /**
     * Return the On/Off times of a PWM channel.
     * 
     * See `set_on_off_times()`.
     */
    expected<Pca9685OnOffTimes, rfs::Error> on_off_times(uint32_t channel) const {
        if (!channel_exists(channel))
            return unexpected(rfs::Error(EINVAL, "channel"));

        const uint8_t reg = channel * NUM_REGISTERS_PER_CHANNEL + CHANNELS_REGISTERS_OFFSET;
        const expected<vector<uint8_t>, rfs::Error> res = read_block(reg, NUM_REGISTERS_PER_CHANNEL);
        if (!res)
            return unexpected(res.error());
        
        const uint16_t on_int = (((*res)[1] & 0x0f) << 8) | (*res)[0];
        const uint16_t off_int = (((*res)[3] & 0x0f) << 8) | (*res)[2];
        return Pca9685OnOffTimes{
            static_cast<float>(on_int)/COUNTER_TICKS,
            static_cast<float>(off_int)/COUNTER_TICKS,
            ((*res)[1] & LED_ON_MASK) ? true : false,
            ((*res)[3] & LED_OFF_MASK) ? true : false
        };
    }

    /**
     * Initializes the communication with a real **PCA9685** device.
     * 
     * After opening, the device is put in *auto increment* mode, which is needed for the rest of the
     * methods to work. Read the device's documentation for more information about this.
     * 
     * `device` is the path in the filesystem of the I<SUP>2</SUP>C controller used to communicate
     * with the **PCA9685** real device. It is usually something like `/dev/i2c-1`.
     * 
     * `address` is the address to identify the **PCA9685** device on the I<SUP>2</SUP>C bus. It is a 7-bit
     * number that uniquely identifies a device on a I<SUP>2</SUP>C bus. Read in the
     * **PCA9685** documentation about this subject, but by default this address is `0x40`.
     */
    expected<void, rfs::Error> open(string device, uint8_t address) {
        fd = ::open(device.c_str(), O_RDWR);
        if (fd < 0) {
            return unexpected(rfs::Error(errno));
        }

        if (ioctl(fd, I2C_SLAVE, address) < 0) {
            ::close(fd);
            fd = -1;
            return unexpected(rfs::Error(errno));
        }
        
        // Enable register address auto-increment
        const expected<void, rfs::Error> result_set_auto_inc = set_auto_increment(true);
        if (!result_set_auto_inc) {
            ::close(fd);
            fd = -1;
        }
        return result_set_auto_inc;
    }

    /**
     * Return the moment at which the **PCA9685** device updates the PWM signals after a configuration change.
     */
    expected<Pca9685OutputChange, rfs::Error> output_change() const {
        const expected<uint8_t, rfs::Error> value = read_register(MODE2_REGISTER);
        if (!value)
            return unexpected(value.error());
        return (*value & MODE2_OCH_MASK) ? Pca9685OutputChange::OnAck : Pca9685OutputChange::OnStop;
    }

    /**
     * Return the state at which the **PCA9685** device sets the PWM outputs when disabled.
     */
    expected<Pca9685OutputDisabledMode, rfs::Error> output_disabled_mode() const {
        const expected<uint8_t, rfs::Error> value = read_register(MODE2_REGISTER);
        if (!value)
            return unexpected(value.error());
        switch (*value & MODE2_OUTNE_MASK) {
            case 0: return Pca9685OutputDisabledMode::Low;
            case 1: return Pca9685OutputDisabledMode::Driver;
            default: return Pca9685OutputDisabledMode::HighImpedance;
        }
    }

    /**
     * Return whether the PWM signals are inverted.
     */
    expected<bool, rfs::Error> output_inverted() const {
        return get_bool(MODE2_REGISTER, MODE2_INVRT_MASK);
    }

    expected<unique_ptr<Pca9685Pwm>, rfs::Error> pwm(uint32_t channel) {
        if (!channel_exists(channel))
            return unexpected(rfs::Error(EINVAL, "channel"));
        weak_ptr<Pca9685PwmInterface> ptr = this_shared;
        return make_unique<Pca9685Pwm>(channel, ptr);
    }

    /**
     * Restart the **PCA9685** after it was put to sleep.
     * 
     * If needed, the restart sequence is executed (to restart the PWM channels).
     * 
     * Returns `true` if the device needed a restart, or `false` in case all the PWM channels
     * where shut down.
     */
    expected<bool, rfs::Error> restart() {
        const expected<bool, rfs::Error> restart_needed = needs_restart();
        if (!restart_needed)
            return restart_needed;
        
        const expected<void, rfs::Error> sleep_released = set_bool(MODE1_REGISTER, MODE1_SLEEP_MASK, false);
        if (!sleep_released)
            return unexpected(sleep_released.error());

        this_thread::sleep_for(chrono::microseconds(500));

        if (*restart_needed) {
            const expected<void, rfs::Error> set_bool_result = set_bool(MODE1_REGISTER, MODE1_RESTART_MASK, true);
            if (!set_bool_result)
                return unexpected(set_bool_result.error());
        }

        return *restart_needed;
    }

    /**
     * Set the ALL_CALL address.
     * 
     * This is an I<SUP>2</SUP>C address to which the **PCA9685** device will also
     * respond to. It is `0xE0` at power-up time.
     * 
     * It has also to be enabled using the `set_all_call_address_enabled()` method.
     */
    expected<void, rfs::Error> set_all_call_address(uint8_t address) {
        return write_register(ALLCALL_REGISTER, address & 0xfe);
    }

    /**
     * Enable responding to the ALL_CALL address.
     * 
     * See `set_all_call_address()`.
     */
    expected<void, rfs::Error> set_all_call_address_enabled(bool enabled) {
        return set_bool(MODE1_REGISTER, MODE1_ALLCALL_MASK, enabled);
    }

    /**
     * Set the PWM signal of a channel to always LOW.
     * 
     * If `enabled` is true, then the PWM signal of the given `channel` will be always off.
     * Otherwise, the On/Off times are considered as usual.
     */
    expected<void, rfs::Error> set_always_off(uint32_t channel, bool enabled) {
        if (!channel_exists(channel))
            return unexpected(rfs::Error(EINVAL));

        const uint8_t reg = channel * NUM_REGISTERS_PER_CHANNEL + CHANNELS_REGISTERS_OFFSET + 3;
        return set_bool(reg, LED_OFF_MASK, enabled);
    }

    /**
     * Set the PWM signal of a channel to always HIGH.
     * 
     * If `enabled` is true, then the PWM signal of the given `channel` will be always on.
     * Otherwise, the On/Off times are considered as usual.
     */
    expected<void, rfs::Error> set_always_on(uint32_t channel, bool enabled) {
        if (!channel_exists(channel))
            return unexpected(rfs::Error(EINVAL));

        const uint8_t reg = channel * NUM_REGISTERS_PER_CHANNEL + CHANNELS_REGISTERS_OFFSET + 1;
        return set_bool(reg, LED_ON_MASK, enabled);
    }

    /**
     * Set whether the **PCA9685** is using an external driver to drive the load.
     */
    expected<void, rfs::Error> set_external_driver(bool enabled) {
        return set_bool(MODE2_REGISTER, MODE2_OUTDRV_MASK, enabled);
    }

    /**
     * Set the frequency of the PWM signal.
     * 
     * The `clock_frequency` parameter has the same meaning as in the `frequency()` method.
     * The range of available frequencies depends on the **PCA9685** device's clock signal. If the
     * given frequency is not valid, an EINVAL error will be returned.
     */
    expected<void, rfs::Error> set_frequency(float frequency, float clock_frequency = INTERNAL_CLOCK_FREQUENCY) {
        if (frequency <= 0.0)
            return unexpected(Error(EINVAL, "frequency"));
        if (clock_frequency < 0.0)
            return unexpected(Error(EINVAL, "clock_frequency"));

        const uint32_t prescale_32 = roundf(clock_frequency/(COUNTER_TICKS*frequency)) - 1;
        if (prescale_32 < MIN_PRESCALE || prescale_32 > MAX_PRESCALE)
            return unexpected(rfs::Error(EINVAL, "prescale value out of range: [" + to_string(MIN_PRESCALE) + ", " + to_string(MAX_PRESCALE)));

        uint8_t prescale = static_cast<uint8_t>(prescale_32);
        const expected<void, rfs::Error> sleep_result = sleep();
        if (!sleep_result)
            return sleep_result;
        
        const expected<void, rfs::Error> write_result = write_register(PRESCALE_REGISTER, prescale);
        if (!write_result)
            return write_result;

        const expected<bool, rfs::Error> restart_result = restart();
        if (!restart_result)
            return unexpected(restart_result.error());
        return {};
    }

    /**
     * Set the On/Off times of the given channel.
     * 
     * `channel` can be a channel number from 1 to 16, or the `ALL_CHANNELS` constant to set the given
     * On/Off times to all the channels.
     * 
     * `on_time` and `off_time` are the position within the signal period where it is turned on and off, respectively.
     * The valid values are from 0.0 to 1.0, 0.0 meaning at the start of the period and 1.0 at the end.
     * `on_time` and `off_time` cannot hold the same value.
     */
    virtual expected<void, rfs::Error> set_on_off_times(uint32_t channel, float on_time, float off_time) override {
        if (!channel_exists(channel))
            return unexpected(rfs::Error(EINVAL, "channel"));
        if (on_time < 0.0 || on_time > 1.0)
            return unexpected(rfs::Error(EINVAL, "on_time"));
        if (off_time < 0.0 || off_time > 1.0)
            return unexpected(rfs::Error(EINVAL, "off_time"));

        const uint16_t on_time_int = min<uint16_t>(static_cast<uint16_t>(on_time * COUNTER_TICKS), COUNTER_TICKS - 1);
        const uint16_t off_time_int = min<uint16_t>(static_cast<uint16_t>(off_time * COUNTER_TICKS), COUNTER_TICKS - 1);

        if (on_time_int == off_time_int)
            return unexpected(rfs::Error(EINVAL, "on_time and off_time must have different values"));

        vector<uint8_t> data{
            static_cast<uint8_t>(on_time_int & 0xff),
            static_cast<uint8_t>((on_time_int >> 8) & 0x0f),
            static_cast<uint8_t>(off_time_int & 0xff),
            static_cast<uint8_t>((off_time_int >> 8) & 0x0f),
        };
        const uint8_t reg = channel * NUM_REGISTERS_PER_CHANNEL + CHANNELS_REGISTERS_OFFSET;
        return write_block(reg, data);
    }

    /**
     * Set the moment at which the **PCA9685** device updates the PWM signals after a configuration change.
     */
    expected<void, rfs::Error> set_output_change(Pca9685OutputChange value) {
        const bool enabled = (value == Pca9685OutputChange::OnAck);
        return set_bool(MODE2_REGISTER, MODE2_OCH_MASK, enabled);
    }

    /**
     * Return the state at which the **PCA9685** device sets the PWM outputs when disabled.
     */
    expected<void, rfs::Error> set_output_disabled_mode(Pca9685OutputDisabledMode value) {
        return set_bits(MODE2_REGISTER, MODE2_OUTNE_MASK, static_cast<uint8_t>(value));
    }

    /**
     * Set whether the PWM signals are inverted.
     */
    expected<void, rfs::Error> set_output_inverted(bool inverted) {
        return set_bool(MODE2_REGISTER, MODE2_INVRT_MASK, inverted);
    }

    /**
     * Set the SUBADDRESS_1.
     * 
     * This is an I<SUP>2</SUP>C address to which the **PCA9685** device will also
     * respond to. It is `0xE2` at power-up time.
     * 
     * It has also to be enabled using the `set_subaddress1_enabled()` method.
     */
    expected<void, rfs::Error> set_subaddress1(uint8_t address) {
        return write_register(SUB1_REGISTER, address & 0xfe);
    }

    /**
     * Set the SUBADDRESS_2.
     * 
     * This is an I<SUP>2</SUP>C address to which the **PCA9685** device will also
     * respond to. It is `0xE4` at power-up time.
     * 
     * It has also to be enabled using the `set_subaddress2_enabled()` method.
     */
    expected<void, rfs::Error> set_subaddress2(uint8_t address) {
        return write_register(SUB2_REGISTER, address & 0xfe);
    }

    /**
     * Set the SUBADDRESS_3.
     * 
     * This is an I<SUP>2</SUP>C address to which the **PCA9685** device will also
     * respond to. It is `0xE8` at power-up time.
     * 
     * It has also to be enabled using the `set_subaddress3_enabled()` method.
     */
    expected<void, rfs::Error> set_subaddress3(uint8_t address) {
        return write_register(SUB3_REGISTER, address & 0xfe);
    }

    /**
     * Enable responding to the SUBADDRESS_1.
     * 
     * See `set_subaddress1()`.
     */
    expected<void, rfs::Error> set_subaddress1_enabled(bool enabled) {
        return set_bool(MODE1_REGISTER, MODE1_SUB1_MASK, enabled);
    }

    /**
     * Enable responding to the SUBADDRESS_2.
     * 
     * See `set_subaddress2()`.
     */
    expected<void, rfs::Error> set_subaddress2_enabled(bool enabled) {
        return set_bool(MODE1_REGISTER, MODE1_SUB2_MASK, enabled);
    }

    /**
     * Enable responding to the SUBADDRESS_3.
     * 
     * See `set_subaddress3()`.
     */
    expected<void, rfs::Error> set_subaddress3_enabled(bool enabled) {
        return set_bool(MODE1_REGISTER, MODE1_SUB3_MASK, enabled);
    }

    /**
     * Put the **PCA9685** in sleep mode.
     */
    expected<void, rfs::Error> sleep() {
        return set_bool(MODE1_REGISTER, MODE1_SLEEP_MASK, true);
    }

    /**
     * Return the SUBADDRESS_1.
     * 
     * See `set_subaddress1()`.
     */
    expected<uint8_t, rfs::Error> subaddress1() const {
        return read_register(SUB1_REGISTER);
    }

    /**
     * Return whether the **PCA9685** device responds to the SUBADDRESS_1 address.
     * 
     * The SUBADDRESS_1 address is disabled at startup.
     * The SUBADDRESS_1 address can be configured using the `set_subaddress1()` method.
     */
    expected<bool, rfs::Error> subaddress1_enabled() const {
        return get_bool(MODE1_REGISTER, MODE1_SUB1_MASK);
    }

    /**
     * Return the SUBADDRESS_2.
     * 
     * See `set_subaddress2()`.
     */
    expected<uint8_t, rfs::Error> subaddress2() const {
        return read_register(SUB2_REGISTER);
    }

    /**
     * Return whether the **PCA9685** device responds to the SUBADDRESS_2 address.
     * 
     * The SUBADDRESS_2 address is disabled at startup.
     * The SUBADDRESS_2 address can be configured using the `set_subaddress2()` method.
     */
    expected<bool, rfs::Error> subaddress2_enabled() const {
        return get_bool(MODE1_REGISTER, MODE1_SUB2_MASK);
    }

    /**
     * Return the SUBADDRESS_3.
     * 
     * See `set_subaddress3()`.
     */
    expected<uint8_t, rfs::Error> subaddress3() const {
        return read_register(SUB3_REGISTER);
    }

    /**
     * Return whether the **PCA9685** device responds to the SUBADDRESS_3 address.
     * 
     * The SUBADDRESS_3 address is disabled at startup.
     * The SUBADDRESS_3 address can be configured using the `set_subaddress3()` method.
     */
    expected<bool, rfs::Error> subaddress3_enabled() const {
        return get_bool(MODE1_REGISTER, MODE1_SUB3_MASK);
    }

private:

    static const int MODE1_RESTART_MASK = 0x80;
    static const int MODE1_EXTCLK_MASK  = 0x40;
    static const int MODE1_AI_MASK      = 0x20;
    static const int MODE1_SLEEP_MASK   = 0x10;
    static const int MODE1_SUB1_MASK    = 0x08;
    static const int MODE1_SUB2_MASK    = 0x04;
    static const int MODE1_SUB3_MASK    = 0x02;
    static const int MODE1_ALLCALL_MASK = 0x01;
    static const int MODE2_INVRT_MASK   = 0x10;
    static const int MODE2_OCH_MASK     = 0x08;
    static const int MODE2_OUTDRV_MASK  = 0x04;
    static const int MODE2_OUTNE_MASK   = 0x03;
    static const int LED_OFF_MASK       = 0x10;
    static const int LED_ON_MASK        = 0x10;

    static const uint8_t MODE1_REGISTER     = 0;
    static const uint8_t MODE2_REGISTER     = 1;
    static const uint8_t SUB1_REGISTER      = 2;
    static const uint8_t SUB2_REGISTER      = 3;
    static const uint8_t SUB3_REGISTER      = 4;
    static const uint8_t ALLCALL_REGISTER   = 4;
    static const uint8_t LED0_REGISTER      = 6;
    static const uint8_t ALL_LED_REGISTER   = 250;
    static const uint8_t PRESCALE_REGISTER  = 254;

    static const uint32_t CHANNELS_COUNT            = 16;
    static const uint8_t NUM_REGISTERS_PER_CHANNEL  = 4;
    static const uint8_t CHANNELS_REGISTERS_OFFSET  = 6;
    static const uint16_t COUNTER_TICKS             = 4096;
    static constexpr float INTERNAL_CLOCK_FREQUENCY = 25e6;
    static const uint8_t MIN_PRESCALE               = 3;
    static const uint8_t MAX_PRESCALE               = 255;

    int fd;
    shared_ptr<Pca9685PwmInterface> this_shared;

    bool channel_exists(uint32_t channel) const {
        return channel < CHANNELS_COUNT || channel == ALL_CHANNELS;
    }

    expected<bool, rfs::Error> get_bool(uint8_t reg, uint8_t mask) const {
        const expected<uint8_t, rfs::Error> value = read_register(reg);
        if (!value)
            return unexpected(value.error());
        return *value & mask;
    }

    expected<vector<uint8_t>, rfs::Error> read_block(uint8_t reg, uint8_t size) const {
        vector<uint8_t> data(size);

        const int32_t read_result = i2c_smbus_read_i2c_block_data(fd, reg, size, data.data());
        if (read_result < 0)
            return unexpected(rfs::Error(errno));
        return data;
    }

    expected<uint8_t, rfs::Error> read_register(uint8_t reg) const {
        const int32_t read_result = i2c_smbus_read_byte_data(fd, reg);
        if (read_result < 0)
            return unexpected(rfs::Error(errno));
        return read_result & 0x00ff;
    }

    expected<void, rfs::Error> set_auto_increment(bool enabled) {
        return set_bool(MODE1_REGISTER, MODE1_AI_MASK, enabled);
    }

    expected<void, rfs::Error> set_bits(uint8_t reg, uint8_t mask, uint8_t value) {
        const expected<uint8_t, rfs::Error> result_read = read_register(reg);
        if (!result_read)
            return unexpected(result_read.error());
        return write_register(reg, value);
    }

    expected<void, rfs::Error> set_bool(uint8_t reg, uint8_t mask, bool value) {
        const expected<uint8_t, rfs::Error> result_read = read_register(reg);
        if (!result_read)
            return unexpected(result_read.error());
        
        const uint8_t register_value = (value ? (*result_read | mask) : (*result_read & ~mask));
        return write_register(reg, register_value);
    }

    expected<void, rfs::Error> write_register(uint8_t reg, uint8_t value) const {
        const int32_t write_result = i2c_smbus_write_byte_data(fd, reg, value);
        if (write_result < 0)
            return unexpected(rfs::Error(errno));
        return {};
    }

    expected<void, rfs::Error> write_block(uint8_t reg, vector<uint8_t> data) {
        const int32_t write_result = i2c_smbus_write_i2c_block_data(fd, reg, data.size(), data.data());
        if (write_result < 0)
            return unexpected(rfs::Error(errno));
        return {};
    }

};

}