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

#include <omp.h>

#include "object.h"

#define _USE_MATH_DEFINES

const double aspectRatio = 4.0 / 3.0;

const glm::vec3 e(0.0, 0.0, 0.0);
const glm::vec3 cameraDirection(0, 0, 1.0);
const glm::vec3 cameraUp(0.0, 1.0, 0.0);
const glm::vec3  cameraRight = glm::cross(cameraUp, cameraDirection);
double top = tan(7*M_PI/72);
double bottom = -top;
double right = top * aspectRatio;
double left = -right;

const glm::vec3 lightPos = e - cameraRight*static_cast<float>(1.9) + cameraUp*static_cast<float>(1.9) ;

// Placeholder foo
glm::vec3 foo = glm::vec3(0.0, 0.0, 0.0);

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

void clamp(glm::vec3& c) {
    // Clamp pixel values
    for (int i = 0; i < 3; i++) {
        if (c[i] < 0) {
            c[i] = 0;
        }
        else if (c[i] > 1) {
            c[i] = 1;
        }
    }
    return;
}

glm::vec3 traceRay(std::vector<Object*> myObjects, glm::vec3 origin, glm::vec3 rayDir, int depth) {
    glm::vec3 c(0.5, 0.0, 1.0);
    glm::vec3 intersectPos(0, 0, 0);
    glm::vec3 normal(0, 0, 0);
    glm::vec3 curIntersectPos(0, 0, 0);
    glm::vec3 curNormal(0, 0, 0);
    float min = 1000;
    int nearestObj = -1;
    bool intersects = false;

    // Loop through spheres checking for intersection with ray direction
    
    for (int k = 0; k < myObjects.size(); k++) {
        float t = myObjects[k]->intersect("Intersection", origin, rayDir, intersectPos, normal);
        if (t > 0) {
            intersects = true;
            if (t < min) {
                min = t;
                nearestObj = k;
                curIntersectPos = intersectPos;
                curNormal = normal;
            }
        }
    }
    c = myObjects[nearestObj]->Color();
    float I_a = myObjects[nearestObj]->AmbientFactor();
    // Display objects, add shading
    if (intersects) {
        bool intersectsShadow = false;
        // Calculate Shadows
        for (int k = 0; k < myObjects.size(); k++) {
            if (k != nearestObj) {
                glm::vec3 shadowRay = lightPos - curIntersectPos;
                float t = myObjects[k]->intersect("Shadow", curIntersectPos, shadowRay, foo, foo);
                if (t == 1) {
                    intersectsShadow = true;
                    break;
                }
            }
        }
        if (depth > 10) {
            return glm::vec3(1.0,1.0,1.0);
        }
        if (myObjects[nearestObj]->Reflect()){
            glm::vec3 reflectedRay = glm::reflect(-glm::normalize(origin - curIntersectPos), curNormal);
            c = traceRay(myObjects, curIntersectPos+0.00001f*reflectedRay, reflectedRay, depth+1);
        }
        if (myObjects[nearestObj]->Refract()) {
            // Assume refractive index of air is 1
            float eta = 1/myObjects[nearestObj]->refractIndex();
            glm::vec3 I = -glm::normalize( origin - curIntersectPos);
            float a = 1-pow(eta,2)*(1-pow(dot(curNormal,I),2));
            if (a < 0) { // Total Internal Reflection
                glm::vec3 internalReflectedRay = glm::reflect(I, curNormal);
                c = traceRay(myObjects, curIntersectPos+0.00001f*internalReflectedRay, internalReflectedRay, depth+1);
            }
            else { // Refraction
                a = pow(a,0.5);
                glm::vec3 refractedRay = (eta*dot(curNormal,I)-a)*curNormal-(eta*I);
                c = traceRay(myObjects, curIntersectPos+0.00001f*refractedRay, refractedRay, depth+1);
            }
        }
        glm::vec3 k_a = c;
        c*=I_a;
        glm::vec3 v = glm::normalize(origin - curIntersectPos);
        float p = myObjects[nearestObj]->SpecularExponent();
        float I_i = 1.0;
        glm::vec3 k_d = k_a;
        glm::vec3 k_s(1.0, 1.0, 1.0);
        glm::vec3 l = glm::normalize(lightPos - curIntersectPos);
        glm::vec3 r_vec = glm::reflect(-l, curNormal);
        if (!intersectsShadow) {
            c += k_s * I_i * static_cast<float>(pow(fmax(0, dot(r_vec, v)), p));
            c += k_d * I_i * static_cast<float>(fmax(0, dot(curNormal, l)));
            clamp(c);
        }
    }
    return c;
}

int main() {
    // image dimensions
    int dimx = 800;
    int dimy = 600;

    std::vector<Object*> myObjects;
    std::vector<Object*> mySpheres;
    Sphere sphereA(glm::vec3(0.0, 0.0, -5.0), 0.75, glm::vec3(1.0, 0.5, 0.0), false, 1.04);
    Sphere sphereB(glm::vec3(1.0, 0.0, -5.5), 0.5, glm::vec3(0.0, 1.0, 0.5), false, AIR_REFRACTIVE_INDEX);
    Sphere sphereC(glm::vec3(-1.0, 0.5, -3.0), 0.2, glm::vec3(0.0f, 0.5, 1.0), false, AIR_REFRACTIVE_INDEX);
    Sphere sphereD(glm::vec3(-0.5f, -0.5f, -2.5f), 0.2, glm::vec3(1.0, 0.5, 0.5), false, AIR_REFRACTIVE_INDEX);

    myObjects.push_back(&sphereA);
    myObjects.push_back(&sphereB);
    myObjects.push_back(&sphereC);
    myObjects.push_back(&sphereD);

    Plane planeA(glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.75, 0.75, 0.75), true, AIR_REFRACTIVE_INDEX);
    Plane planeB(glm::vec3(-1.0, 0.0, 0.0), glm::vec3(2.0, 0.0, 0.0), glm::vec3(0.75, 0.75, 0.75), false, AIR_REFRACTIVE_INDEX);
    Plane planeC(glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -10.0), glm::vec3(0.75, 0.75, 0.75), false, AIR_REFRACTIVE_INDEX);
    Plane planeD(glm::vec3(1.0, 0.0, 0.0), glm::vec3(-3.0, 0.0, 0.0), glm::vec3(0.75, 0.75, 0.75), false, AIR_REFRACTIVE_INDEX);
    Plane planeE(glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 2.5, 0.0), glm::vec3(0.75, 0.75, 0.75), false, AIR_REFRACTIVE_INDEX);
    Plane planeF(glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 0.0, 2.0), glm::vec3(0.75, 0.75, 0.75), false,AIR_REFRACTIVE_INDEX);

    myObjects.push_back(&planeA);
    myObjects.push_back(&planeB);
    myObjects.push_back(&planeC);
    myObjects.push_back(&planeD);
    myObjects.push_back(&planeE);
    myObjects.push_back(&planeF);

    int numSpheres = 4;

//    Object *mySphere = myObjects[0];
//    std::cout << mySphere->intersect(foo, foo, foo, foo) << std::endl;
    //std::cout << myObjects[0]->intersect(foo, foo, foo, foo) << std::endl;

    // image data
    std::vector<glm::u8vec3> image;
    image.resize(dimx * dimy);
    std::vector<glm::vec3> rayDirs;
    rayDirs.resize(dimx * dimy);
    // start time measurement
    auto start = std::chrono::high_resolution_clock::now();

    // ASSIGNMENT 1 (RAY VIEWS)
//
//        for (int j = dimy-1; j >= 0; --j) {
//        for (int i = 0; i < dimx; ++i) {
//            // Calculate ray directions for each pixel
//            auto u = glm::vec3(left + (right-left)*(i+0.5)/dimx);
//            auto v = glm::vec3(-bottom + (-top+bottom)*(j+0.5)/dimy);
//            auto r = glm::normalize(-cameraDirection + u*cameraRight + v*cameraUp - e);
//            //glm::vec3 c = traceRay(myObjects, e, r, 0);
//            image[j*dimx + i] = static_cast<glm::u8vec3>(r*glm::vec3(255));
//        }
//    }

    // ASSIGNMENT 2: SPHERES
//    for (int j = dimy-1; j >= 0; --j) {
//        for (int i = 0; i < dimx; ++i) {
//            // Calculate ray directions for each pixel
//            auto u = glm::vec3(left + (right - left) * (i + 0.5) / dimx);
//            auto v = glm::vec3(-bottom + (-top + bottom) * (j + 0.5) / dimy);
//            auto r = glm::normalize(-cameraDirection + u * cameraRight + v * cameraUp - e);
//            glm::vec3 intersectPos(0, 0, 0);
//            glm::vec3 normal(0, 0, 0);
//            glm::vec3 curIntersectPos(0, 0, 0);
//            glm::vec3 curNormal(0, 0, 0);
//            float min = 1000;
//            int nearestObj = -1;
//            bool intersects = false;
//
//            // Loop through spheres checking for intersection with ray direction
//            for (int k = 0; k < numSpheres; k++) {
//                float t = myObjects[k]->intersect("Intersection", e, r, intersectPos, normal);
//                if (t > 0) {
//                    intersects = true;
//                    if (t < min) {
//                        min = t;
//                        nearestObj = k;
//                        curIntersectPos = intersectPos;
//                        curNormal = normal;
//                    }
//                }
//            }
//            glm::vec3 c(0.5, 0.0, 1.0);
//            if (intersects) {
//                c = myObjects[nearestObj]->Color();
//            }
//            image[j * dimx + i] = static_cast<glm::u8vec3>(c * glm::vec3(255));
//        }
//    }

//     ASSIGNMENT 3-4: SHADING & SHADOWS
//    for (int j = dimy-1; j >= 0; --j) {
//        for (int i = 0; i < dimx; ++i) {
//            // Calculate ray directions for each pixel
//            auto u = glm::vec3(left + (right - left) * (i + 0.5) / dimx);
//            auto v = glm::vec3(-bottom + (-top + bottom) * (j + 0.5) / dimy);
//            auto r = glm::normalize(-cameraDirection + u * cameraRight + v * cameraUp - e);
//            glm::vec3 intersectPos(0, 0, 0);
//            glm::vec3 normal(0, 0, 0);
//            glm::vec3 curIntersectPos(0, 0, 0);
//            glm::vec3 curNormal(0, 0, 0);
//            float min = 1000;
//            int nearestObj = -1;
//            bool intersects = false;
//
//            // Loop through spheres checking for intersection with ray direction
//            for (int k = 0; k < numSpheres; k++) {
//                float t = myObjects[k]->intersect("Intersection", e, r, intersectPos, normal);
//                if (t > 0) {
//                    intersects = true;
//                    if (t < min) {
//                        min = t;
//                        nearestObj = k;
//                        curIntersectPos = intersectPos;
//                        curNormal = normal;
//                    }
//                }
//            }
//            glm::vec3 c(0.5, 0.0, 1.0);
//            if (intersects) {
//                c = myObjects[nearestObj]->Color();
//            }
//
//            // Display objects, add shading
//            if (intersects) {
//                bool intersectsShadow = false;
//                 // Calculate Shadows
//                 // ASSIGNMENT 4: UNCOMMENT FROM HERE
////                for (int k = 0; k < numSpheres; k++) {
////                    if (k != nearestObj) {
////                        glm::vec3 shadowRay = lightPos - curIntersectPos;
////                        float t = myObjects[k]->intersect("Shadow", curIntersectPos, shadowRay, foo, foo);
////                        if (t == 1) {
////                            intersectsShadow = true;
////                            break;
////                        }
////                    }
////                }
//                // TO HERE FOR SHADOWS
//                glm::vec3 k_a = c;
//                c*=0.2;
//                glm::vec3 v = glm::normalize(e - curIntersectPos);
//                float p = myObjects[nearestObj]->SpecularExponent();
//                float I_i = 1.0;
//                glm::vec3 k_d = k_a;
//                glm::vec3 k_s(1.0, 1.0, 1.0);
//                glm::vec3 l = glm::normalize(lightPos - curIntersectPos);
//                glm::vec3 r_vec = glm::reflect(-l, curNormal);
//                if (!intersectsShadow) {
//                    c += k_s * I_i * static_cast<float>(pow(fmax(0, dot(r_vec, v)), p));
//                    c += k_d * I_i * static_cast<float>(fmax(0, dot(curNormal, l)));
//                    clamp(c);
//                }
//            }
//            image[j * dimx + i] = static_cast<glm::u8vec3>(c * glm::vec3(255));
//        }
//    }


    // ASSIGNMENT 5 (Planes): Set k<myObjects.size() in intersection and shadow
    // ASSIGNMENT 6 (Reflection): Set Sphere A and Plane A to true
    // EC (Refraction): Reset AIR_REFRACTIVE_INDEX to 1.52
    for (int j = dimy - 1; j >= 0; --j) {
            for (int i = 0; i < dimx; ++i) {
                // Calculate ray directions for each pixel
                auto u = glm::vec3(left + (right - left) * (i + 0.5) / dimx);
                auto v = glm::vec3(-bottom + (-top + bottom) * (j + 0.5) / dimy);
                auto r = glm::normalize(-cameraDirection + u * cameraRight + v * cameraUp - e);
                glm::vec3 c = traceRay(myObjects, e, r, 0);
                image[j * dimx + i] = static_cast<glm::u8vec3>(c * glm::vec3(255));
            }
        }

   // stop time
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "Total execution time in milliseconds: " << duration.count() << std::endl;

    // write image data to PPM file
    return writeP6PPM((unsigned int) dimx, (unsigned int) dimy, image);
}