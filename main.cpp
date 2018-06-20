#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "linalg.h"

#include <vector>
#include <random>
#include <iostream>
#include <chrono>

using namespace std;
using namespace linalg::aliases;

struct Ray {
    float3 org;
    float3 dir;
    Ray(float3 o, float3 d) : org(o), dir(d){}
};

struct Sphere {
    float3 pos;
    float radius;
    Sphere(float3 p, float r) : pos(p), radius(r){}
    double intersect(const Ray& ray)
    {
        float3 op = pos - ray.org;
        double t;
        const double eps = 1e-4;
        double b = dot(op, ray.dir);
        double det = b * b - dot(op, op) + radius * radius;
        if (det < 0.0f) {
            return 0;
        } else {
            det = sqrt(det);
        }
        return (t = b - det) > eps ? t : (( t= b + det) > eps ? t : 0);
    }
};

int main(int argc, char** argv)
{
    const size_t width = 512;
    const size_t height = 512;

    vector<byte3> pixels(width * height);

    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> dist(0, 255);

    Sphere sphere(float3(50.0, 52.0, 0.0f), 20);

    auto start = chrono::steady_clock::now();

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float3 eye(50.0f, 52.0f, 295.6f);
            float3 dir = normalize(float3(0.0f, -0.042612f, -1.0f));

            float3 cx(width * 0.5135f / height, 0.0f, 0.0f);
            float3 cy = normalize(cross(cx, dir)) * float3(0.5135f);

            float3 d = cx * float3((x - width  / 2.0f) / width) +
                       cy * float3((y - height / 2.0f) / height) +
                       dir;

            Ray ray(eye + d * float3(140.0f), d);

            double t = sphere.intersect(ray);

            pixels[x + y * width] = t != 0.0 ? byte3(255, 255, 255) : byte3(0, 0, 0);
        }
    }

    auto end = chrono::steady_clock::now();
    auto diff = end - start;

    std::cout << "Elapsed " << chrono::duration_cast<chrono::milliseconds>(diff).count() << " [ms]" << std::endl;

    stbi_write_png("iq.png", width, height, 3, pixels.data(), width * 3);
    
    return 0;
}