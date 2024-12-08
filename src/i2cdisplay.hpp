
#pragma once

#include <array>
#include <string_view>

namespace rfs {


template <typename T>
class i2cdisplay
{

// Private static constants

private:

    // Commands
    static constexpr uint8_t CLEAR_DISPLAY = 0x01;
    static constexpr uint8_t RETURN_HOME = 0x02;
    static constexpr uint8_t ENTRY_MODE_SET = 0x04;
    static constexpr uint8_t DISPLAY_CONTROL = 0x08;
    static constexpr uint8_t CURSOR_SHIFT = 0x10;
    static constexpr uint8_t FUNCTION_SET = 0x20;
    static constexpr uint8_t SET_CGRAM_ADDR = 0x40;
    static constexpr uint8_t SET_DDRAM_ADDR = 0x80;

    // Flags for display entry mode set
    static constexpr uint8_t ENTRY_LEFT = 0x02;

    // Flags for display on/off control
    static constexpr uint8_t DISPLAY_ON = 0x04;
    static constexpr uint8_t CURSOR_ON = 0x02;
    static constexpr uint8_t BLINK_ON = 0x01;

    // Flags for function set
    static constexpr uint8_t LINE_2 = 0x08;

    // Flags for backlight control
    static constexpr uint8_t BACKLIGHT = 0x08;

    // flags for display/cursor shift
    static constexpr uint8_t DISPLAY_MOVE = 0x08;
    static constexpr uint8_t MOVE_RIGHT = 0x04;

    // Special flags
    static constexpr uint8_t ENABLE = 0x04;
    static constexpr uint8_t REGISTER_SELECT = 0x01;
    static constexpr uint8_t CHAR = 0x01;
    static constexpr unsigned int DELAY_US = 600;
    static constexpr uint8_t MAX_ROWS = 4;


// Public static constants

public:

    static constexpr uint8_t MAX_CUSTOM_CHARS = 8;
    static constexpr uint8_t CUSTOM_SYMBOL_SIZE = 8;


// Private members

private:

    uint8_t rows;
    uint8_t columns;
    T i2c_driver;
    uint8_t backlight;
    uint8_t display_mode;
    uint8_t display_control;


// Private methods

private:

    bool write_to_i2c(uint8_t value) const
    {
        return i2c_driver.write_byte(value | backlight);
    }

    bool send_enable_pulse(uint8_t value) const
    {
        usleep(DELAY_US);
        const bool result_enable = write_to_i2c(value | ENABLE);
        usleep(DELAY_US);
        const bool result_disable = write_to_i2c(value & ~ENABLE);
        usleep(DELAY_US);

        return result_enable && result_disable;
    }

    bool write_nibble(uint8_t value) const
    {
        return write_to_i2c(value) && send_enable_pulse(value);
    }

    bool write_byte(uint8_t value, uint8_t mode) const
    {
        const uint8_t high_nibble = value & 0xf0;
        const uint8_t low_nibble = (value << 4) & 0xf0;

        return write_nibble(high_nibble | mode) && write_nibble(low_nibble | mode);
    }

    bool send_command(uint8_t command) const
    {
        return write_byte(command, 0);
    }


// Public functions

public:

    i2cdisplay(uint8_t rows, uint8_t columns, const T &i2c_driver):
        rows(std::min(rows, MAX_ROWS)),
        columns(columns),
        i2c_driver(i2c_driver),
        backlight(0),
        display_mode(ENTRY_LEFT),
        display_control(DISPLAY_ON)
    {

    }

    ~i2cdisplay()
    {

    }

    bool init()
    {
        uint8_t display_function = LINE_2;

        bool result = send_command(0x03);
        result &= send_command(0x03);
        result &= send_command(0x03);
        result &= send_command(0x02);

        result &= send_command(ENTRY_MODE_SET | display_mode);
        result &= send_command(FUNCTION_SET | display_function);
        result &= set_display_on();
        result &= clear();
        result &= go_home();

        return result;
    }

    bool clear() const
    {
        return send_command(CLEAR_DISPLAY);
    }

    bool create_char(uint8_t slot, const std::array<uint8_t, CUSTOM_SYMBOL_SIZE> &char_map) const
    {
        slot = std::min(MAX_CUSTOM_CHARS, slot);
        bool result = send_command(SET_CGRAM_ADDR | (slot << 3));
        for (uint8_t c: char_map) {
            result &= write_byte(c, REGISTER_SELECT);
        }
        return result;
    }

    bool go_home() const
    {
        return send_command(RETURN_HOME);
    }

    bool print(char character) const
    {
        return write_byte(character, CHAR);
    }

    bool print(std::string_view str) const
    {
        bool result = true;
        for (const char c: str)
        {
            result &= print(c);
        }
        return result;
    }

    bool scroll_left() const
    {
        return send_command(CURSOR_SHIFT | DISPLAY_MOVE);
    }

    bool scroll_right() const
    {
        return send_command(CURSOR_SHIFT | DISPLAY_MOVE | MOVE_RIGHT);
    }

    /*bool turn_off_autoscroll()
    {
        display_mode &= ~static_cast<std::underlying_type_t<display_entry_mode_flags>>(display_entry_mode_flags::entry_shift_increment);
        return command(display_commands::entry_mode_set | display_mode);
    }

    bool turn_on_autoscroll()
    {
        display_mode |= static_cast<std::underlying_type_t<display_entry_mode_flags>>(display_entry_mode_flags::entry_shift_increment);
        return command(display_commands::entry_mode_set | display_mode);
    }*/

    bool set_backlight_off()
    {
        backlight = 0;
        return write_to_i2c(backlight);
    }

    bool set_backlight_on()
    {
        backlight = BACKLIGHT;
        return write_to_i2c(backlight);
    }

    bool set_blink_off()
    {
        display_control &= ~BLINK_ON;
        return send_command(DISPLAY_CONTROL | display_control);
    }

    bool set_blink_on()
    {
        display_control |= BLINK_ON;
        return send_command(DISPLAY_CONTROL | display_control);
    }

    bool set_cursor_off()
    {
        display_control &= ~CURSOR_ON;
        return send_command(DISPLAY_CONTROL | display_control);
    }

    bool set_cursor_on()
    {
        display_control |= CURSOR_ON;
        return send_command(DISPLAY_CONTROL | display_control);
    }

    bool set_cursor_position(uint8_t row, uint8_t column) const
    {
        const std::array ROW_OFFSETS = {0x00, 0x40, 0x00 + columns, 0x40 + columns};

        row = std::min(rows, row);
        column = std::min(columns, column);
        return send_command(SET_DDRAM_ADDR | (ROW_OFFSETS[row] + column));
    }

    bool set_display_off()
    {
        display_control &= ~DISPLAY_ON;
        return send_command(DISPLAY_CONTROL | display_control);
    }

    bool set_display_on()
    {
        display_control |= DISPLAY_ON;
        return send_command(DISPLAY_CONTROL | display_control);
    }

    bool set_text_left_to_right()
    {
        display_mode |= ENTRY_LEFT;
        return send_command(ENTRY_MODE_SET | display_mode);
    }

    bool set_text_right_to_left()
    {
        display_mode &= ~ENTRY_LEFT;
        return send_command(ENTRY_MODE_SET | display_mode);
    }

};

}