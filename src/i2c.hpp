
#pragma once

extern "C" {
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace rfs {

class i2c
{

private:

    int fd;

public:

    i2c(): fd(-1) {}

    ~i2c()
    {
        if (fd > -1) {
            close();
        }
    }

    bool close()
    {
        if (::close(fd) == -1) {
            return false;
        }
        fd = -1;
        return true;
    }

    bool open(const char *device, uint8_t address)
    {
        fd = ::open(device, O_RDWR);
        if (fd < 0) {
            return false;
        }
        if (ioctl(fd, I2C_SLAVE, address) < 0) {
            ::close(fd);
            fd = -1;
            return false;
        }
        return true;
    }

    uint8_t read_block(uint8_t reg, uint8_t size, uint8_t *data, bool &error) const
    {
        const int read_result = i2c_smbus_read_i2c_block_data(fd, reg, size, data);
        error = read_result < 0;
        return read_result;
    }

    uint8_t read_register(uint8_t reg, uint8_t size, bool &error) const
    {
        const int read_result = i2c_smbus_read_byte_data(fd, reg);
        error = read_result < 0;
        return read_result & 0x00ff;
    }

    uint8_t write_block(uint8_t reg, const uint8_t *data, uint8_t size, bool &error) const
    {
        const int write_result = i2c_smbus_write_i2c_block_data(fd, reg, size, data);
        error = write_result < 0;
        return write_result;
    }

    bool write_byte(uint8_t value) const
    {
        return i2c_smbus_write_byte(fd, value) >= 0;
    }

    bool write_register(uint8_t reg, uint8_t value) const
    {
        return i2c_smbus_write_byte_data(fd, reg, value) >= 0;
    }

};

}