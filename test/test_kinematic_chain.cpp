
#include <glm/gtx/string_cast.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <vector>

#include "../src/kinematics.hpp"

using namespace glm;
using namespace rfs;
using namespace std;

int main()
{
    KinematicChain k({{0.0, M_PI/2.0, 32.2}, {0.0, 0.0, 48.6}, {0.0, 0.0, 113.713}});

    // Try forward kinematics
    cout << "FORWARD KINEMATICS" << endl;
    cout << "==================" << endl;
    const vector<float> angles({0.0, 0.0, -M_PI/2.0});
    cout << "joint angles: ";
    for (const float angle: angles) {
        cout << angle << " ";
    }
    cout << endl;
    k.set_angles(angles);

    const vec3 ee_pos = k.forward_kinematics();
    cout << "end effector position: " << ee_pos[0] << ", " << ee_pos[1] << ", " << ee_pos[2] << endl;
    cout << endl;

    // Try inverse kinematics
    cout << "INVERSE KINEMATICS" << endl;
    cout << "==================" << endl;
    const vec3 ee_pos_inv{100.0, 0.0, -30.0};
    cout << "target: " << to_string(ee_pos_inv) << endl;
    k.inverse_kinematics(ee_pos_inv);

    cout << "joint angles: ";
    for (const float angle: k.get_angles()) {
        cout << angle << " ";
    }
    cout << endl;

    const vec3 ee_pos_after = k.forward_kinematics();
    cout << "end effector position: " << to_string(ee_pos_after) << endl;
}