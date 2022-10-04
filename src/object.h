#ifndef OBJECT_H_
#define OBJECT_H_

#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

class Object {
public:
	Object(glm::vec3 col, bool reflecting = false, float ambientFactor = 0.2f, float specExponent = 50.0f) { 
		color = col; 
		reflect = reflecting;
		ambient = ambientFactor; 
		specularEx = specExponent; 
	};
	// intersection function: returns the closest intersection point with the given ray (or a negative value, if none exists)
	// output parameters: location of the intersection, object normal
	// PURE VIRTUAL FUNCTION: has to be implemented in all child classes.
    virtual float intersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& intersectPos, glm::vec3& normal) = 0;

	const glm::vec3& Color() { return color; };
	float AmbientFactor() { return ambient; };
	float SpecularExponent() { return specularEx; };
	bool Reflect() { return reflect; };

private:
	// object color
	glm::vec3 color;
	// basic material parameters
	float ambient;
	float specularEx;
	// is this object reflecting?
	bool reflect;
};

class Sphere:public Object {
public:
    Sphere(glm::vec3 c, double r, glm::vec3 col, bool reflecting = false, float ambientFactor = 0.2f, float specExponent = 50.0f): Object(col, reflecting = false, ambientFactor = 0.2f, specExponent = 50.0f) {
        center = c;
        radius = r;
        color = col;
    }
    virtual float intersect(const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& intersectPos, glm::vec3& normal) {
        glm::vec3 oc = rayOrigin - center;
        auto a = glm::dot(rayDir, rayDir);
        auto b = 2.0 * glm::dot(oc, rayDir);
        auto c = glm::dot(oc, oc) - radius*radius;
        auto discriminant = b*b - 4*a*c;
        if (discriminant >= 0) {
            float t = std::min((glm::dot(-rayDir, oc) + sqrt(discriminant))/a,(glm::dot(-rayDir, oc) - sqrt(discriminant))/a);
            intersectPos = rayOrigin + t*rayDir;
            normal = glm::normalize((intersectPos - center)*static_cast<float>(2));
            return t;
        } else {
            return -1;
        }
    }
    glm::vec3 shading(const glm::vec3& v, const glm::vec3& lightPosition, const glm::vec3& n, const glm::vec3& intersectPos) {
        float p = 500.0;
        float I_i = 1.0;
        float I_a = 0.2;
        glm::vec3 k_a = color;
        glm::vec3 k_d = color;
        glm::vec3 k_s(1.0, 1.0, 1.0);
        glm::vec3 g = glm::vec3(0,0,0) - intersectPos;
        glm::vec3 l = glm::normalize(lightPosition - intersectPos);
        glm::vec3 h = glm::normalize(g + l);
        glm::vec3 L = k_a*I_a + k_d*I_i*static_cast<float>(fmax(0, dot(n,l))) + k_s*I_i*static_cast<float>(pow(fmax(0,dot(n,h)),p));
        for (int i = 0; i < 3; i++) {
            if (L[i] < 0) {
                L[i] = 0;
            }
            else if (L[i] > 1) {
                L[i] = 1;
            }
        }
        return L;
    }
private:
    glm::vec3(color);
    glm::vec3(center);
    double radius;
};
#endif
