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

    double top = tan(M_PI/8);
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
            glm::vec3 cur_intersectPos(0, 0, 0);
            glm::vec3 cur_normal(0, 0, 0);
            float min = 10;
            int closest_sphere = -1;
            bool intersects = false;

            // Loop through spheres checking for intersection with ray direction
            for (int k = 0; k < spheres.size(); k++) {
                float t = spheres[k].intersect(e, r, intersectPos, normal);
                if (t != -1) {
                    intersects = true;
                    if (t < min) {
                        min = t;
                        closest_sphere = k;
                        cur_intersectPos = intersectPos;
                        cur_normal = normal;
                    }
                }
            }
            // Display spheres, add shading
            if (!intersects) {
                auto background_color = glm::vec3(0.5, 0.0, 1.0);
                image[j*dimx + i] = static_cast<glm::u8vec3>(background_color*glm::vec3(255));
            }
            else {
                auto L = spheres[closest_sphere].shading(cameraDirection, lightPos, cur_normal, cur_intersectPos);
                image[j*dimx + i] = static_cast<glm::u8vec3>(L*glm::vec3(255));
                //auto p = spheres[closest_sphere].Color();
                //newImage[j*dimx + i] = static_cast<glm::u8vec3>(p*glm::vec3(255));
            }
        }
    }

//    for (int j = dimy-1; j >= 0; --j) {
//        for (int i = 0; i < dimx; ++i) {
//            auto u = glm::vec3(left + (right-left)*(i+0.5)/dimx);
//            auto v = glm::vec3(-bottom + (-top+bottom)*(j+0.5)/dimy);
//            auto r = glm::normalize(-cameraDirection + u * cameraRight + v * cameraUp - e);
//            rayDirs[j*dimx + i] = r;
//            image[j*dimx + i] = static_cast<glm::u8vec3>(r*glm::vec3(255));
//        }
//    }
//
//    std::vector<Sphere> spheres;
//    spheres.push_back(Sphere(glm::vec3(0.0, 0.0, -5.0), 0.75, glm::vec3(1.0, 0.5, 0.0)));
//    spheres.push_back(Sphere(glm::vec3(1.0, 0.0, -5.5), 0.5, glm::vec3(0.0, 1.0, 0.5)));
//    spheres.push_back(Sphere(glm::vec3(-1.0, 0.5, -3.0), 0.2, glm::vec3(0.0f, 0.5, 1.0)));
//    spheres.push_back(Sphere(glm::vec3(-0.5f, -0.5f, -2.5f), 0.2, glm::vec3(1.0, 0.5, 0.5)));
//
//    std::vector<glm::u8vec3> newImage;
//    newImage.resize(dimx * dimy);
//    glm::vec3 intersectPos(0, 0, 0);
//    glm::vec3 normal(0, 0, 0);
//
//    glm::vec3 cur_intersectPos(0, 0, 0);
//    glm::vec3 cur_normal(0, 0, 0);
//    for (int j = dimy-1; j >= 0; --j) {
//        for (int i = 0; i < dimx; ++i) {
//            float min = 10;
//            bool intersects = false;
//            int closest_sphere = -1;
//            for (int k = 0; k < spheres.size(); k++) {
//                float t = spheres[k].intersect(e, rayDirs[j*dimx + i], intersectPos, normal);
//                if (t != -1) {
//                    intersects = true;
//                    if (t < min) {
//                        min = t;
//                        closest_sphere = k;
//                        cur_intersectPos = intersectPos;
//                        cur_normal = normal;
//                    }
//                }
//            }
//            if (!intersects) {
//                auto background_color = glm::vec3(0.5, 0.0, 1.0);
//                newImage[j*dimx + i] = static_cast<glm::u8vec3>(background_color*glm::vec3(255));
//            }
//            else {
//                auto L = spheres[closest_sphere].shading(cameraUp, lightPos, cur_normal, cur_intersectPos);
//                newImage[j*dimx + i] = static_cast<glm::u8vec3>(L*glm::vec3(255));
//                //auto p = spheres[closest_sphere].Color();
//                //newImage[j*dimx + i] = static_cast<glm::u8vec3>(p*glm::vec3(255));
//            }
//        }
//    }

    // write image data to PPM file
    return writeP6PPM((unsigned int) dimx, (unsigned int) dimy, image);

    // stop time
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "Total execution time in milliseconds: " << duration.count() << std::endl;

}