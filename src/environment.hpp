
#pragma once

#include <algorithm>
#include <array>
#include <climits>
#include <string>

using namespace std;

string read_env(const string &env, const string &default_value = "")
{
    const char *env_value = getenv(env.c_str());

    if (env_value != nullptr) {
        return env_value;
    } else {
        return default_value;
    }
}

bool read_env_bool(const string &env, bool default_value = false)
{
    const char *env_value = getenv(env.c_str());

    if (env_value != nullptr) {
        array<string, 7> list_values = {"true", "True", "yes", "Yes", "y", "Y", "1"};
        return find(begin(list_values), end(list_values), env_value) != end(list_values);
    } else {
        return default_value;
    }
}

long read_env_long(const string &env, long default_value = 0, long min_value = LONG_MIN, long max_value = LONG_MAX)
{
    const char *env_value = getenv(env.c_str());

    if (!env_value)
        return default_value;

    long value = default_value;
    try {
        value = stol(env_value, nullptr, 0);

        if (value < min_value || value > max_value) {
            value = default_value;
        }
    } catch (const exception &e) {}

    return value;
}

long read_env_long_indexed(const string &env, uint32_t index, long default_value = 0, long min_value = LONG_MIN, long max_value = LONG_MAX)
{
    const string env_indexed = string(env) + "_" + to_string(index);
    return read_env_long(env_indexed);
}

float read_env_float(const string &env, float default_value = 0.0)
{
    const char *env_value = getenv(env.c_str());
    try {
        return stof(env_value);
    } catch (const exception &e) {
        return default_value;
    }
}

