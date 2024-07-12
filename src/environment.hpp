
#pragma once

#include <algorithm>
#include <array>
#include <string>

using namespace std;

string read_env(const string &env, const string &default_value)
{
    const char *env_value = getenv(env.c_str());

    if (env_value != nullptr) {
        return env_value;
    } else {
        return default_value;
    }
}

bool read_env_bool(const string &env, bool default_value)
{
    const char *env_value = getenv(env.c_str());

    if (env_value != nullptr) {
        array<string, 7> list_values = {"true", "True", "yes", "Yes", "y", "Y", "1"};
        return find(begin(list_values), end(list_values), env_value) != end(list_values);
    } else {
        return default_value;
    }
}