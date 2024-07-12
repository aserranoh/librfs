
#pragma once

#include <expected>
#include <zmq.hpp>

#include "error.hpp"

using namespace std;

namespace rfs {

class ServoCommand {

private:

    static constexpr int PREFIX_LENGTH = 5;

public:

    ServoCommand(int32_t id, float angle):
        msg_prefix{'S', 'E', 'R', 'V', 'O'}, id{id}, angle{angle}
    {}

    static expected<ServoCommand, Error> from_zmq_message(const zmq::message_t &message)
    {
        if (message.size() != sizeof(ServoCommand)) {
            return unexpected(Error(EBADMSG, "wrong message size"));
        }

        ServoCommand cmd = *(ServoCommand *)message.data();

        if (string(cmd.msg_prefix, 5) != prefix()) {
            return unexpected(Error(EBADMSG, "wrong prefix"));
        }
        return cmd;
    }

    static string prefix()
    {
        return "SERVO";
    }

    string to_string() const
    {
        return "SERVO(id=" + std::to_string(id) + ", angle=" + std::to_string(angle) + ")";
    }

    zmq::message_t to_zmq_message() const
    {
        zmq::message_t message{sizeof(ServoCommand)};
        *(ServoCommand *)message.data() = *this;
        return message;
    }

    char msg_prefix[PREFIX_LENGTH];
    int32_t id;
    float angle;

};

}