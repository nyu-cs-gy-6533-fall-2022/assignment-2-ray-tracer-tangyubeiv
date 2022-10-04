#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <vector>
#include <chrono>
#include <string>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "object.h"

#define _USE_MATH_DEFINES

// write PPM image file (input parameters: image width in px, image height in px, image data (vector of uint8 vec3), filename)
bool writeP6PPM(unsigned int dX, unsigned int dY, std::vector<glm::u8vec3> img, std::string filename = "rtimage") {
    // return false if image size does not fit data size
    try {
        if (img.size() != dX * dY) return false;
        // PPM writing code adapted from: https://rosettacode.org/wiki/Bitmap/Write_a_PPM_file#C++
        std::ofstream ofs(filename + ".ppm", std::ios_base::out | std::ios_base::binary);
        if (ofs.fail()) return false;
        ofs << "P6" << std::endl << dX << ' ' << dY << std::endl << "255" << std::endl;
        for (unsigned int j = 0; j < dY; ++j)
            for (unsigned int i = 0; i < dX; ++i)
                ofs << img[j * dX + i].r << img[j * dX + i].g << img[j * dX + i].b;
        ofs.close();
    }
    catch (const std::exception& e) {
        // return false if an exception occurred
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

int main() {
    // image dimensions
    int dimx = 800;
    int dimy = 600;

    // TODO Set up camera, light etc.
    double aspectRatio = 4.0 / 3.0;

    auto e = glm::vec3(0.0, 0.0, 0.0);
    auto cameraDirection = glm::vec3(0, 0, 1.0);
    auto cameraUp = glm::vec3(0.0, 1.0, 0.0);
    auto cameraRight = glm::cross(cameraUp, cameraDirection);
    //auto cameraRight = glm::vec3(0.3,0.2,0.1);

    double top = tan(7*M_PI/72);
    double bottom = -top;
    double right = top * aspectRatio;
    double left = -right;

    auto lightPos = e - cameraRight*static_cast<float>(1.9) + cameraUp*static_cast<float>(1.9) ;

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(glm::vec3(0.0, 0.0, -5.0), 0.75, glm::vec3(1.0, 0.5, 0.0)));
    spheres.push_back(Sphere(glm::vec3(1.0, 0.0, -5.5), 0.5, glm::vec3(0.0, 1.0, 0.5)));
    spheres.push_back(Sphere(glm::vec3(-1.0, 0.5, -3.0), 0.2, glm::vec3(0.0f, 0.5, 1.0)));
    spheres.push_back(Sphere(glm::vec3(-0.5f, -0.5f, -2.5f), 0.2, glm::vec3(1.0, 0.5, 0.5)));


    // image data
    std::vector<glm::u8vec3> image;
    image.resize(dimx * dimy);
    std::vector<glm::vec3> rayDirs;
    rayDirs.resize(dimx * dimy);
    // start time measurement
    auto start = std::chrono::high_resolution_clock::now();
    // TODO Loop over all pixels
    // TODO    Compute ray-object intersections etc.
    // TODO    Store color per pixel in image vector

    for (int j = dimy-1; j >= 0; --j) {
        for (int i = 0; i < dimx; ++i) {
            auto u = glm::vec3(left + (right-left)*(i+0.5)/dimx);
            auto v = glm::vec3(-bottom + (-top+bottom)*(j+0.5)/dimy);
            auto r = glm::normalize(-cameraDirection + u*cameraRight + v*cameraUp - e);

            glm::vec3 intersectPos(0, 0, 0);
            glm::vec3 normal(0, 0, 0);
            glm::vec3 curIntersectPos(0, 0, 0);
            glm::vec3 curNormal(0, 0, 0);
            float min = 10;
            int nearestSphere = -1;
            bool intersects = false;

            // Loop through spheres checking for intersection with ray direction
            for (int k = 0; k < spheres.size(); k++) {
                float t = spheres[k].intersect(e, r, intersectPos, normal);
                if (t != -1) {
                    intersects = true;
                    if (t < min) {
                        min = t;
                        nearestSphere = k;
                        curIntersectPos = intersectPos;
                        curNormal = normal;
                    }
                }
            }
            // Display spheres, add shading
            if (!intersects) {
                auto background_color = glm::vec3(0.5, 0.0, 1.0);
                image[j*dimx + i] = static_cast<glm::u8vec3>(background_color*glm::vec3(255));
            }
            else {
                float p = 500.0;
                float I_i = 1.0;
                float I_a = 0.2;
                glm::vec3 k_a = spheres[nearestSphere].Color();
                glm::vec3 k_d = k_a;
                glm::vec3 k_s(1.0, 1.0, 1.0);
                glm::vec3 g = glm::vec3(0,0,0) - curIntersectPos;
                glm::vec3 l = glm::normalize(lightPos - curIntersectPos);
                glm::vec3 h = glm::normalize(g + l);
                glm::vec3 c =  k_a*I_a;
                bool intersectsShadow = false;
                for (int k = 0; k < spheres.size(); k++) {
                    if (k != nearestSphere) {
                        float m = 0.01;
                        glm::vec3 shadowRay = m * l;
                        float t = spheres[k].intersect(curIntersectPos, shadowRay, normal, curIntersectPos);
                        if (t != -1) {
                            intersectsShadow = true;
                            break;
                        }
                    }
                }
                if (!intersectsShadow) {
                    c = c + k_d*I_i*static_cast<float>(fmax(0, dot(curNormal, l))) + k_s * I_i * static_cast<float>(pow(fmax(0, dot(curNormal, h)), p));
                }
                // Clamp pixel values
                for (int i = 0; i < 3; i++) {
                    if (c[i] < 0) {
                        c[i] = 0;
                    }
                    else if (c[i] > 1) {
                        c[i] = 1;
                    }
                }
                image[j*dimx + i] = static_cast<glm::u8vec3>(c*glm::vec3(255));
            }
        }
    }

    // write image data to PPM file
    return writeP6PPM((unsigned int) dimx, (unsigned int) dimy, image);

    // stop time
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "Total execution time in milliseconds: " << duration.count() << std::endl;

}