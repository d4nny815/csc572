#include <memory>
#include <iostream>
#include <filesystem>
#include "camera.h"
#include "plane.h"
#include "scene.h"
#include "sphere.h"
#include "triangle.h"
#include "obj_loader.h"

namespace fs = std::filesystem;
namespace rt { void render_scene_ppm(const Scene&, const Camera&, int, int, const std::string&); }

int main(int argc, char** argv){
    using namespace rt;
    const int W = 800, H = 600, SPP = 16;
    Camera cam({0,2.5,8.5}, {0,1,0}, {0,1,0}, 45.0, W, H, 0.0, 6);

    Scene sc;
    int matGrey  = sc.addMaterial(std::make_shared<Material>(Vec3{0.8, 0.8, 0.8}));
    sc.add(std::make_unique<Plane>(Vec3{0, 0, 0}, Vec3{0, 1, 0}, matGrey));
    sc.lights.push_back({Vec3{0, 5, 7}, Vec3{50, 50, 50}});

    std::string objPath = (argc > 1) ? 
        std::string(argv[1]) : 
        std::string("../assets/sphere_low.obj");
    
    std::vector<Vec3> V;
    std::vector<uint32_t> I;
    
    bool loaded = fs::exists(objPath) && load_obj_positions_indices(objPath, V, I) && !V.empty() && !I.empty();
    if (loaded) {
        std::cerr << "Loaded OBJ: " << objPath << "  V=" << V.size() << "  T=" << I.size()/3 << "\n";
        int matBunny = sc.addMaterial(std::make_shared<Material>(Vec3{0.8, 0.8, 0.9}));
        add_mesh(sc, V, I, matBunny, /*scale*/{1, 1, 1}, /*translate*/{0, 0.6, 0});
    } else {
        std::cerr << "OBJ not found or failed to load. Proceeding without mesh.\n";
    }

    // Add spheres
    int matRed   = sc.addMaterial(std::make_unique<Material>(Vec3{0.8, 0.2, 0.2}));
    int matBlue  = sc.addMaterial(std::make_unique<Material>(Vec3{0.2, 0.2, 0.8}));
    int matGold = sc.addMaterial(std::make_unique<Metal>(Vec3((float)0xD4/255.0, (float)0xAF/255.0, (float)0x37/255.0), 0.05));
    int matMirror = sc.addMaterial(std::make_unique<Metal>(Vec3(0.9, 0.9, 0.95), 0.0));
    
    // sc.add(std::make_unique<Sphere>(Vec3{-3.0, 1.0, 0.0}, 1.0, matMirror));
    // sc.add(std::make_unique<Sphere>(Vec3{0.0, 1.0, 2.0}, 1.0, matBlue));
    sc.add(std::make_unique<Sphere>(Vec3{3.0, 3.0, 2.5}, 1.0, matGold));
    // sc.add(std::make_unique<Sphere>(Vec3{0.0, 3.5, -2.0}, .5, matRed));

    render_scene_ppm(sc, cam, SPP, 10, "out.ppm");
    return 0;
}
