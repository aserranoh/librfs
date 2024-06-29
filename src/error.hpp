
#pragma once

#include <cstring>
#include <string>

using namespace std;

namespace rfs {

/**
 * TODO
 */
class Error {

public:

    Error(int error_code): error_code(error_code) {}
    Error(int error_code, const string &detail): error_code(error_code), detail_msg(detail) {}

    string detail() const {
        if (!detail_msg.empty())
            return string{strerror(error_code)} + ": " + detail_msg;
        else
            return string{strerror(error_code)};
    }

    string name() const {
        return {strerrorname_np(error_code)};
    }

private:

    int error_code;
    string detail_msg;

};

}