#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "linalg.h"
#include "stb_image_write.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

using namespace std;
using namespace linalg::aliases;

struct Ray;
struct HitInfo;
struct Sphere;
class Material;
class Lambertian;
class Camera;
class World;

namespace iq {
float random() {
  static std::default_random_engine engine;
  static std::uniform_real_distribution<float> distribution(0, 1);
  return distribution(engine);
}

float3 randomInUnitSphere() {
  float u = iq::random();
  float v = iq::random();

  const double pi = 3.14159265358979323846;
  float theta = u * 2.0f * static_cast<float>(pi);
  float phi = acos(2.0f * v - 1.0f);
  float r = cbrt(iq::random());

  float sinTheta = sin(theta);
  float cosTheta = cos(theta);
  float sinPhi = sin(phi);
  float cosPhi = cos(phi);

  float x = r * sinPhi * cosTheta;
  float y = r * sinPhi * sinTheta;
  float z = r * cosPhi;

  return float3(x, y, z);
}

float3 randomInUnitDisk() {
  float2 point(randomInUnitSphere().xy());
  return float3(point.x, point.y, 0.0);
}

} // namespace iq

struct Ray {
  float3 org;
  float3 dir;
  Ray() {}
  Ray(float3 o, float3 d) : org(o), dir(d) {}
  float3 pointAt(const float t) const { return org + t * dir; }
};

struct HitInfo {
  float t;
  float3 p;
  float3 normal;
  Material *material;
  HitInfo(float t, float3 p, float3 normal, Material *material)
      : t(t), p(p), normal(normal), material(material) {}
};

struct Sphere {
  float3 m_pos;
  float m_radius;
  std::shared_ptr<Material> m_material;
  Sphere(float3 p, float r, std::shared_ptr<Material> material)
      : m_pos(p), m_radius(r), m_material(std::move(material)) {}
  std::optional<HitInfo> intersect(const Ray &ray, float tmin,
                                   float tmax) const {
    const float3 oc = ray.org - m_pos;
    const float a = dot(ray.dir, ray.dir);
    const float b = dot(oc, ray.dir);
    const float c = dot(oc, oc) - m_radius * m_radius;
    const float discriminant = b * b - a * c;
    if (discriminant > 0.0f) {
      float temp = (-b - sqrt(discriminant)) / a;
      if (temp < tmax && temp > tmin) {
        const float3 _pos = ray.pointAt(temp);
        auto info = std::make_optional<HitInfo>(
            temp, _pos, (_pos - m_pos) / m_radius, m_material.get());
        return info;
      }
      temp = (-b + sqrt(discriminant)) / a;
      if (temp < tmax && temp > tmin) {
        const float3 _pos = ray.pointAt(temp);
        auto info = std::make_optional<HitInfo>(
            temp, _pos, (_pos - m_pos) / m_radius, m_material.get());
        return info;
      }
    }
    return {};
  }
};

class Material {
public:
  virtual ~Material() {}
  virtual bool scatter(const Ray &in, const HitInfo &info, float3 &attenuation,
                       Ray &scattered) const = 0;
};

class Lambertian : public Material {
public:
  Lambertian(const float3 &albedo) : m_albedo(albedo) {}
  virtual bool scatter(const Ray &in, const HitInfo &info, float3 &attenuation,
                       Ray &scattered) const {
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
  Camera(float3 eye, float3 at, float3 up, float fov, float aspect,
         float aperture, float focusDist) {
    m_lensRadius = aperture / 2.0f;

    const double pi = 3.14159265358979323846;

    const float theta = fov * static_cast<float>(pi) / 180.0f;
    const float half_height = tanf(theta / 2.0f);
    const float half_width = aspect * half_height;

    m_origin = eye;

    m_w = normalize(eye - at);
    m_u = normalize(cross(up, m_w));
    m_v = cross(m_w, m_u);

    m_lowerLeftCorner = m_origin - half_width * focusDist * m_u -
                        half_height * focusDist * m_v - focusDist * m_w;
    m_horizontal = 2.0f * half_width * focusDist * m_u;
    m_vertical = 2.0f * half_height * focusDist * m_v;
  }
  Ray generate(float s, float t) {
    float3 rd = m_lensRadius * iq::randomInUnitDisk();
    float3 offset = m_u * rd.x + m_v * rd.y;
    return Ray(m_origin + offset,
               normalize(m_lowerLeftCorner + s * m_horizontal + t * m_vertical -
                         m_origin - offset));
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
  std::optional<HitInfo> intersect(const Ray &ray, const float tmin,
                                   const float tmax) const {
    std::optional<HitInfo> nearest;
    float closest = tmax;
    for (size_t i = 0; i < m_spheres.size(); ++i) {
      const Sphere *sphere = m_spheres[i].get();
      if (auto local = sphere->intersect(ray, tmin, closest)) {
        closest = local->t;
        nearest = local;
      }
    }

    return nearest;
  }
  void add(shared_ptr<Sphere> sphere) {
    m_spheres.push_back(std::move(sphere));
  }

private:
  std::vector<shared_ptr<Sphere>> m_spheres;
};

float3 radiance(const Ray &ray, const World &world, int depth) {
  const float tmin = numeric_limits<float>::min();
  const float tmax = numeric_limits<float>::max();
  const int maxDepth = 16;

  if (auto info = world.intersect(ray, tmin, tmax)) {
    Ray scattered;
    float3 attenuation;
    if (depth < maxDepth &&
        info->material->scatter(ray, *info, attenuation, scattered)) {
      return attenuation * radiance(scattered, world, depth + 1);
    } else {
      return float3(0.0f, 0.0f, 0.0f);
    }
  } else {
    float3 unitDirection = normalize(ray.dir);
    float t = 0.5f * (unitDirection.y + 1.0f);
    return float3(1.0f - t) * float3(1.0f, 1.0f, 1.0f) +
           float3(t) * float3(0.1f, 0.1f, 0.1f);
  }
}

int main(int argc, char **argv) {
  const size_t width = 800;
  const size_t height = 600;
  const size_t samples = 8;

  vector<byte3> pixels(width * height);
  vector<double3> accumulation(width * height);

  const float3 eye(0.0f, 2.0f, 3.0f);
  const float3 at(0.0f, 0.0f, 0.0f);
  const float3 up(0.0f, -1.0f, 0.0f);

  const float focusDist = 3.0f;
  const float aperture = 0.0f;
  const float aspect = float(width) / float(height);
  const float fov = 40.0f;

  Camera camera(eye, at, up, fov, aspect, aperture, focusDist);

  vector<shared_ptr<Material>> materials;
  materials.push_back(make_shared<Lambertian>(float3(0.75f, 0.75f, 0.75f)));
  materials.push_back(make_shared<Lambertian>(float3(0.8f, 0.8f, 0.9f)));
  materials.push_back(make_shared<Lambertian>(float3(0.0f, 1.0f, 0.0f)));
  materials.push_back(make_shared<Lambertian>(float3(1.0f, 0.0f, 0.0f)));
  materials.push_back(make_shared<Lambertian>(float3(1.0f, 1.0f, 1.0f)));

  vector<shared_ptr<Sphere>> spheres;
  spheres.push_back(
      make_shared<Sphere>(float3(0.0f, -100.5f, -1.0f), 100.0f, materials[0]));
  spheres.push_back(
      make_shared<Sphere>(float3(1.0f, 0.0f, -1.0f), 0.5f, materials[1]));
  spheres.push_back(
      make_shared<Sphere>(float3(0.0f, 0.0f, -1.0f), 0.5f, materials[2]));
  spheres.push_back(
      make_shared<Sphere>(float3(-1.0f, 0.0f, -1.0f), 0.5f, materials[3]));
  spheres.push_back(
      make_shared<Sphere>(float3(0.0f, 0.0f, 0.0f), 0.5f, materials[4]));

  World world;
  for (const auto sphere : spheres) {
    world.add(sphere);
  }

  auto start = chrono::steady_clock::now();

  for (size_t s = 1; s < samples + 1; ++s) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        const float u = float(x + iq::random()) / float(width);
        const float v = float(y + iq::random()) / float(height);

        const Ray ray = camera.generate(u, v);
        const float3 rgb = radiance(ray, world, 0);

        const float3 color = float3(sqrt(rgb[0]), sqrt(rgb[1]), sqrt(rgb[2]));
        accumulation[x + y * width] += double3(color);
      }
    }

    for (size_t i = 0; i < accumulation.size(); ++i) {
      const double3 value = accumulation[i];
      const double3 color = value * double3(1.0f / s);

      pixels[i] =
          byte3(255.0f * color[0], 255.0f * color[1], 255.0f * color[2]);
    }

    stbi_write_png("iq.png", width, height, 3, pixels.data(), width * 3);
  }

  auto end = chrono::steady_clock::now();
  auto diff = end - start;

  std::cout << "Elapsed "
            << chrono::duration_cast<chrono::milliseconds>(diff).count()
            << " [ms]" << std::endl;

  return 0;
}