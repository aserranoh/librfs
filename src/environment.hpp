
#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <nlohmann/json.hpp>

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

uint8_t read_env_u8(const string &env, uint8_t default_value = 0, uint8_t min_value = 0, uint8_t max_value = 255)
{
    const char *env_value = getenv(env.c_str());

    if (!env_value)
        return default_value;

    unsigned long ulong_value = default_value;
    try {
        ulong_value = stoul(env_value, nullptr, 0);

        if (ulong_value < min_value || ulong_value > max_value) {
            ulong_value = default_value;
        }
    } catch (const exception &e) {}

    return ulong_value;
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

nlohmann::json read_env_json(const string &env, const nlohmann::json &default_value = {})
{
    const char *env_value = getenv(env.c_str());

    try {
        return nlohmann::json::parse(env_value);
    } catch (const nlohmann::json::parse_error &error) {
        return default_value;
    }
}