
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <ranges>
#include <vector>

using namespace glm;
using namespace std;

namespace rfs {

struct KinematicChainJoint {
    float angle;
    vec3 position;
    vec3 z;
};

struct DHParameters {
    float d;
    float alpha;
    float r;
};

class KinematicChain {

public:

    KinematicChain(const vector<DHParameters> &parameters):
        parameters(parameters), joints(parameters.size())
    {}

    ~KinematicChain()
    {}

    vec3 forward_kinematics()
    {
        mat4 T(1.0);
        vec3 pos{0.0, 0.0, 0.0};
        vec3 z{0.0, 0.0, 1.0};

        for (tuple<DHParameters&, KinematicChainJoint&> elem: views::zip(parameters, joints)) {
            DHParameters &params = get<0>(elem);
            KinematicChainJoint &joint = get<1>(elem);
            joint.position = pos;
            joint.z = z;

            // BEWARE! in GLM the matrix are specified COLUMN-WISE!
            const mat4 Z{{ cos(joint.angle), sin(joint.angle),      0.0, 0.0},
                         {-sin(joint.angle), cos(joint.angle),      0.0, 0.0},
                         {              0.0,              0.0,      1.0, 0.0},
                         {              0.0,              0.0, params.d, 1.0}};
            
            const mat4 X{{     1.0,                0.0,               0.0, 0.0},
                         {     0.0,  cos(params.alpha), sin(params.alpha), 0.0},
                         {     0.0, -sin(params.alpha), cos(params.alpha), 0.0},
                         {params.r,                0.0,               0.0, 1.0}};
            
            T *= Z * X;
            pos = {T[3].x, T[3].y, T[3].z};
            z = mat3(T) * vec3(0.0, 0.0, 1.0);
        }
        return pos;
    }

    vector<float> get_angles() const {
        vector<float> angles;
        for (const KinematicChainJoint &joint: joints) {
            angles.push_back(joint.angle);
        }
        return angles;
    }

    void inverse_kinematics(const vec3 &target, int max_iterations = MAX_ITERATIONS, float max_error = 1.0)
    {
        vec3 ee_pos = forward_kinematics();
        int iteration = 0;
        
        while (iteration < max_iterations && distance(ee_pos, target) > max_error) {
            for (auto joint = joints.rbegin(); joint != joints.rend(); joint++) {
                joint->angle += compute_signed_angle(ee_pos, target, *joint);
                ee_pos = forward_kinematics();
            }
            iteration++;
        }
    }

    void set_angles(const vector<float> &angles)
    {
        for (tuple<KinematicChainJoint&, float> elem: views::zip(joints, angles)) {
            KinematicChainJoint &joint = get<0>(elem);
            float angle = get<1>(elem);
            joint.angle = angle;
        }
    }

private:

    float compute_signed_angle(const vec3 &ee_pos, const vec3 &target, const KinematicChainJoint &joint)
    {
        const vec3 n1 = normalize(cross(joint.z, ee_pos - joint.position));
        const vec3 n2 = normalize(cross(joint.z, target - joint.position));
        const float angle = acos(dot(n1, n2));
        const vec3 z_2 = cross(n1, n2);

        return (dot(joint.z, z_2) > 0) ? angle : -angle;
    }

    static constexpr int MAX_ITERATIONS = 128;

    vector<DHParameters> parameters;
    vector<KinematicChainJoint> joints;

};

}