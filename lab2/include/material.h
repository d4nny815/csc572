#pragma once
#include "vec3.h"
#include "ray.h"

namespace rt {

struct Hit; // Forward declaration

struct Material {
    Vec3 albedo;
    Material(Vec3 a = {0.8, 0.8, 0.8}) : albedo(a) {}
    virtual ~Material() = default;
    virtual bool scatter(const Ray& r_in, const Hit& hit, Vec3& attenuation, Ray& scattered) const {
        return false;
    }
};

inline Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - 2 * dot(v, n) * n;
}

inline Vec3 random_in_unit_sphere() {
    while (true) {
        Vec3 p = Vec3(drand48(), drand48(), drand48()) * 2.0 - Vec3(1, 1, 1);
        if (dot(p, p) < 1.0) return p;
    }
}

class Metal : public Material {
public:
    float fuzz;
    Metal(const Vec3& a, float f) : Material(a), fuzz(f < 1 ? f : 1) {}
    bool scatter(const Ray& r_in, const Hit& hit, Vec3& attenuation, Ray& scattered) const override {
        Vec3 reflected = reflect(normalize(r_in.d), hit.n);
        Vec3 fuzzed = reflected + fuzz * random_in_unit_sphere();
        scattered = Ray(hit.p, fuzzed);
        attenuation = albedo;
        return dot(scattered.d, hit.n) > 0;
    }
};

} // namespace rt