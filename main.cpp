#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "linalg.h"

#include <vector>
#include <random>
#include <iostream>
#include <chrono>

using namespace std;
using namespace linalg::aliases;

struct Ray;
struct HitInfo;
struct Sphere;
class Material;
class Lambertian;
class Camera;
class World;

namespace iq
{
    float random()
    {
        static std::default_random_engine engine;
        static std::uniform_real_distribution<> distribution(0, 1);
        return distribution(engine);
    }

    float3 randomInUnitSphere()
    {
        float3 p;
        do {
            p = float3(2.0f) * float3(iq::random(), iq::random(), iq::random()) - float3(1.0f, 1.0f, 1.0f);
        } while (dot(p,p) >= 1.0f);
        return p;
    }

    float3 randomInUnitDisk()
    {
        float3 p;
        do {
            p = float3(2.0) * float3(iq::random(), iq::random(), 0.0f) - float3(1.0f, 1.0f, 0.0f);
        } while (dot(p,p) >= 1.0);
        return p;
    }
}

struct Ray {
    float3 org;
    float3 dir;
    Ray() {}
    Ray(float3 o, float3 d) : org(o), dir(d) {}
    float3 pointAt(const float t) const
    {
        return org + t * dir;
    }
};

struct HitInfo {
    float t;
    float3 p;
    float3 normal;
    Material* material;
};

struct Sphere {
    float3 m_pos;
    float m_radius;
    Material* m_material;
    Sphere(float3 p, float r, Material* material) : m_pos(p), m_radius(r), m_material(material) {}
    bool intersect(const Ray& ray, float tmin, float tmax, HitInfo& info) const
    {
        const float3 oc = ray.org - m_pos;
        const float a = dot(ray.dir, ray.dir);
        const float b = dot(oc, ray.dir);
        const float c = dot(oc, oc) - m_radius * m_radius;
        const float discriminant = b*b - a*c;
        if (discriminant > 0.0f) {
            float temp = (-b - sqrt(discriminant))/a;
            if (temp < tmax && temp > tmin) {
                info.t = temp;
                info.p = ray.pointAt(info.t);
                info.normal = (info.p - m_pos) / m_radius;
                info.material = m_material;
                return true;
            }
            temp = (-b + sqrt(discriminant)) / a;
            if (temp < tmax && temp > tmin) {
                info.t = temp;
                info.p = ray.pointAt(info.t);
                info.normal = (info.p - m_pos) / m_radius;
                info.material = m_material;
                return true;
            }
        }
        return false;
    }
};

class Material {
public:
    virtual bool scatter(const Ray& in, const HitInfo& info, float3& attenuation, Ray& scattered) const = 0;
};

class Lambertian : public Material {
public:
    Lambertian(const float3& albedo) : m_albedo(albedo) {}
    virtual bool scatter(const Ray& in, const HitInfo& info, float3& attenuation, Ray& scattered) const
    {
        const float3 target = info.p + info.normal + iq::randomInUnitSphere();
        scattered = Ray(info.p, target - info.p);
        attenuation = m_albedo;
        return true;
    }
private:
    float3 m_albedo;
};

class Camera {
 public:
    Camera(float3 eye, float3 at, float3 up, float fov, float aspect, float aperture, float focusDist)
    {
        m_lensRadius = aperture / 2.0f;

        const double pi = 3.14159265358979323846f;

        const float theta = fov * pi/180.0f;
        const float half_height = tan(theta/2);
        const float half_width = aspect * half_height;

        m_origin = eye;

        m_w = normalize(eye - at);
        m_u = normalize(cross(up, m_w));
        m_v = cross(m_w, m_u);

        m_lowerLeftCorner = m_origin  - half_width * focusDist * m_u -half_height * focusDist * m_v - focusDist * m_w;
        m_horizontal = 2.0f * half_width * focusDist * m_u;
        m_vertical = 2.0f * half_height * focusDist * m_v;
    }
    Ray generate(float s, float t)
    {
        float3 rd = m_lensRadius * iq::randomInUnitDisk();
        float3 offset = m_u * rd.x + m_v * rd.y;
        return Ray(m_origin + offset, m_lowerLeftCorner + s*m_horizontal + t*m_vertical - m_origin - offset);
    }
private:
    float3 m_origin;
    float3 m_lowerLeftCorner;
    float3 m_horizontal;
    float3 m_vertical;
    float3 m_u, m_v, m_w;
    float m_lensRadius;
};

class World {
public:
    bool intersect(const Ray& ray, const float tmin, const float tmax, HitInfo& info) const
    {
        bool hit = false;
        double closest = tmax;
        HitInfo temp;
        for (size_t i = 0; i < m_spheres.size(); ++i)
        {
            const Sphere* sphere = m_spheres[i];
            if (sphere->intersect(ray, tmin, closest, temp))
            {
                hit = true;
                closest = temp.t;
                info = temp;
            }
        }

        return hit;
    }
    void add(const Sphere* sphere)
    {
        m_spheres.push_back(sphere);
    }
private:
    std::vector<const Sphere*> m_spheres;
};


float3 radiance(const Ray& ray, const World& world, int depth)
{
    const float tmin = numeric_limits<float>::min();
    const float tmax = numeric_limits<float>::max();
    const int maxDepth = 16;

    HitInfo info;
    if (world.intersect(ray, tmin, tmax, info))
    {
        Ray scattered;
        float3 attenuation;
        if (depth < maxDepth && info.material->scatter(ray, info, attenuation, scattered))
        {
            return attenuation * radiance(scattered, world, depth + 1);
        }
        else
        {
            return float3(0.0f, 0.0f, 0.0f);
        }
    }
    else
    {
        float3 unitDirection = normalize(ray.dir);
        float t = 0.5f * (unitDirection.y + 1.0f);
        return float3(1.0 - t) * float3(1.0f, 1.0f, 1.0f) + float3(t) * float3(0.1f, 0.1f, 0.1f);
    }
}

int main(int argc, char** argv)
{
    const size_t width   = 800;
    const size_t height  = 600;
    const size_t samples =   8;

    vector<byte3> pixels(width * height);

    const float3 eye(0.0f, 2.0f, 3.0f);
    const float3 at(0.0f, 0.0f, 0.0f);

    const float focusDist = 3.0f;
    const float aperture = 0.0f;
    const float aspect = float(width) / float(height);
    const float fov = 40.0f;

    Camera camera(eye, at, float3(0.0f, -1.0f, 0.0f), fov, aspect, aperture, focusDist);

    World world;
    world.add (new Sphere(float3( 0.0f, -100.5f, -1.0f), 100.0f, new Lambertian(float3(0.75f, 0.75f, 0.75f))));
    world.add (new Sphere(float3( 1.0f,    0.0f, -1.0f),   0.5f, new Lambertian(float3(1.0f, 1.0f, 1.0f))));
    world.add (new Sphere(float3( 0.0f,    0.0f, -1.0f),   0.5f, new Lambertian(float3(0.0f, 1.0f, 0.0f))));
    world.add (new Sphere(float3(-1.0f,    0.0f, -1.0f),   0.5f, new Lambertian(float3(1.0f, 1.0f, 1.0f))));

    auto start = chrono::steady_clock::now();

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float3 rgb(0.0f, 0.0f, 0.0f);
            for (size_t s = 0; s < samples; ++s)
            {
                const float u = float(x + iq::random()) / float(width);
                const float v = float(y + iq::random()) / float(height);
                const Ray ray = camera.generate(u, v);

                rgb += radiance(ray, world, 0);
            }
            rgb = rgb * float3(1.0f / samples);
            const float3 color = float3(sqrt(rgb[0]), sqrt(rgb[1]), sqrt(rgb[2]));

            pixels[x + y * width] = byte3(255.0f * color[0], 255.0f * color[1], 255.0f * color[2]);
        }
    }

    auto end = chrono::steady_clock::now();
    auto diff = end - start;

    std::cout << "Elapsed " << chrono::duration_cast<chrono::milliseconds>(diff).count() << " [ms]" << std::endl;

    stbi_write_png("iq.png", width, height, 3, pixels.data(), width * 3);
    
    return 0;
}