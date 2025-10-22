#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>

#include "camera.h"
#include "scene.h"

namespace rt {

struct RNG {
    std::mt19937_64 gen;
    std::uniform_real_distribution<double> dist;

    RNG(uint64_t seed = 0)
        : gen(seed ? seed : std::chrono::high_resolution_clock::now().time_since_epoch().count()),
          dist(0.0, 1.0) {}

    double uniform() { return dist(gen); }
};

class Renderer {
public:
    Renderer(const Scene& s, const Camera& c, int spp_ = 16, double gamma_ = 2.2, int maxDepth_ = 10)
        : scene(s), cam(c), spp(spp_), gamma(gamma_), eps(1e-4), maxDepth(maxDepth_) {}

    void renderPPM(const std::string& filename) const {
        std::ofstream out(filename, std::ios::binary);
        out << "P6\n" << cam.W << " " << cam.H << "\n255\n";

        RNG rng;
        for (int j = cam.H - 1; j >= 0; --j) {
            std::cerr << "Row " << (cam.H - 1 - j) << "/" << cam.H << "\r";

            for (int i = 0; i < cam.W; ++i) {
                Vec3 col(0);

                for (int s = 0; s < spp; ++s) {
                    double u = ((i + rng.uniform()) / double(cam.W)) * 2.0 - 1.0;
                    double v = ((j + rng.uniform()) / double(cam.H)) * 2.0 - 1.0;
                    Ray pr = cam.primary(u, v);
                    col += trace(pr, maxDepth);
                }

                col = col / double(spp);
                col = { col.x / (1.0 + col.x), 
                        col.y / (1.0 + col.y), 
                        col.z / (1.0 + col.z) };

                unsigned char r = (unsigned char)(std::clamp(toSRGB(col.x), 0.0, 1.0) * 255.99);
                unsigned char g = (unsigned char)(std::clamp(toSRGB(col.y), 0.0, 1.0) * 255.99);
                unsigned char b = (unsigned char)(std::clamp(toSRGB(col.z), 0.0, 1.0) * 255.99);

                out.write((char*)&r, 1);
                out.write((char*)&g, 1);
                out.write((char*)&b, 1);
            }
        }
        std::cerr << "\nWrote " << filename << "\n";
    }

private:
    const Scene& scene;
    const Camera& cam;
    int spp;
    double gamma;
    double eps;
    int maxDepth = 5;

    bool inShadow(const Vec3& p, const Vec3& n, const PointLight& L) const {
        Vec3 toL = L.pos - p;
        double distL = length(toL);
        Vec3 dir = toL / distL;
        Ray shadowRay(p + n * eps, dir);
        Hit h;

        if (scene.intersect(shadowRay, 0.0, distL - 1e-5, h))
            return true;

        return false;
    }

    Vec3 shade(const Hit& h, const Ray& r, int depth) const {
        const Material* m = scene.materials[h.matId].get();
        Vec3 c(0);

        Vec3 attenuation;
        Ray scattered(Vec3(0), Vec3(1,0,0));
        if (depth > 0 && m->scatter(r, h, attenuation, scattered)) {
            c += hadamard(attenuation, trace(scattered, depth - 1));
        } else {
            for (const auto& L : scene.lights) {
                if (inShadow(h.p, h.n, L)) continue;

                Vec3 wi = normalize(L.pos - h.p);
                double ndotl = std::max(0.0, dot(h.n, wi));
                Vec3 Li = L.intensity / (4.0 * M_PI * dot(L.pos - h.p, L.pos - h.p));

                c += hadamard(m->albedo, Li) * ndotl;
            }
        }

        return c;
    }

    Vec3 trace(const Ray& r, int depth) const {
        if (depth <= 0) return Vec3(0);

        Hit h;
        if (scene.intersect(r, 1e-6, 1e9, h))
            return shade(h, r, depth);

        Vec3 u = normalize(r.d);
        double t = 0.5 * (u.y + 1.0);
        return (1.0 - t) * Vec3(1, 1, 1) + t * Vec3(0.6, 0.8, 1.0);
    }

    double toSRGB(double c) const {
        c = std::max(0.0, c);
        return std::pow(c, 1.0 / gamma);
    }
};

} // namespace rt

namespace rt {
    void render_scene_ppm(const Scene& sc, const Camera& cam, int spp, int maxDepth, const std::string& outPath) {
        Renderer r(sc, cam, spp, 2.2, maxDepth);
        r.renderPPM(outPath);
    }
}