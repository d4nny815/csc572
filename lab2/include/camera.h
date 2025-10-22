#pragma once
#include <cmath>
#include "vec3.h"
#include "ray.h"

namespace rt {

constexpr double PI = 3.14159265358979323846;

inline Vec3 random_in_unit_disk() {
    double r = std::sqrt((double)rand() / RAND_MAX);
    double theta = 2 * PI * ((double)rand() / RAND_MAX);
    return Vec3(r * std::cos(theta), r * std::sin(theta), 0);
}


struct Camera {
    Vec3 eye, look, up;
    double vfov;
    int W, H;

    Vec3 u, v, w;
    double aspect, halfH, halfW;

    double aperture;
    double focus_dist;
    Vec3 lens_u, lens_v;

    Camera(const Vec3& eye_, const Vec3& look_, const Vec3& up_, double vfov_deg, int W_, int H_,
           double aperture_ = 0.0, double focus_dist_ = 10.0)
        : eye(eye_), look(look_), up(up_),
          vfov(vfov_deg * (PI / 180.0)), W(W_), H(H_),
          aperture(aperture_), focus_dist(focus_dist_) {
        aspect = double(W) / H;
        halfH = std::tan(vfov / 2.0);
        halfW = aspect * halfH;

        w = normalize(eye - look);
        u = normalize(cross(up, w));
        v = cross(w, u);

        double lens_radius = aperture / 2.0;
        lens_u = u * lens_radius;
        lens_v = v * lens_radius;
    }

    Ray primary(double sx, double sy) const {
        Vec3 dir = normalize(-w + sx * halfW * u + sy * halfH * v);

        if (aperture > 0.0) {
            Vec3 lens_offset = lens_u * random_in_unit_disk().x + lens_v * random_in_unit_disk().y;
            Vec3 origin = eye + lens_offset;
            Vec3 focus_point = eye + dir * focus_dist;
            Vec3 new_dir = normalize(focus_point - origin);
            return Ray(origin, new_dir);
        } else {
            return Ray(eye, dir);
        }
    }
};


} // namespace rt
