#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <vector>
#include <random>

using namespace std;

int main(int argc, char** argv)
{
    const size_t width = 512;
    const size_t height = 512;

    vector<uint8_t> pixels(width * height * 3);

    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> dist(0, 255);

    for (size_t i = 0; i < pixels.size(); ++i)
    {
        pixels[i] = dist(gen);
    }

    stbi_write_png("iq.png", width, height, 3, pixels.data(), width * 3);
    
    return 0;
}