#ifndef OBJECT_H_
#define OBJECT_H_

#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

const float AIR_REFRACTIVE_INDEX = 1.0003;

class Object {
public:
	Object(glm::vec3 col, bool reflecting, float refractIndex, float ambientFactor = 0.2f, float specExponent = 50.0f) {
		color = col; 
		reflect = reflecting;
		ambient = ambientFactor; 
		specularEx = specExponent;
        eta = refractIndex;
        if (eta != AIR_REFRACTIVE_INDEX) {
            refract = true;
        }
        else {
            refract = false;
        }

	};
	// intersection function: returns the closest intersection point with the given ray (or a negative value, if none exists)
	// output parameters: location of the intersection, object normal
	// PURE VIRTUAL FUNCTION: has to be implemented in all child classes.
    virtual float intersect(std::string type, const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& intersectPos, glm::vec3& normal) = 0;
	const glm::vec3& Color() { return color; };
    const glm::vec3& newColor() { std::cout << color[0] << " " << color[1] << " " << color[2] << std::endl; return color; };
	float AmbientFactor() { return ambient; };
	float SpecularExponent() { return specularEx; };
	bool Reflect() { return reflect; };
    bool Refract() { return refract; };
    float refractIndex() {return eta; };
private:
	// object color
	glm::vec3 color;
	// basic material parameters
	float ambient;
	float specularEx;
	// is this object reflecting?
	bool reflect;
    bool refract;
    float eta;
};

class Sphere:public Object {
public:
    Sphere(glm::vec3 c, double r, glm::vec3 col, bool reflecting, float refractIndex, float ambientFactor = 0.2f, float specExponent = 50.0f): Object(col, reflecting, refractIndex, ambientFactor = 0.2f, specExponent = 50.0f) {
        center = c;
        radius = r;
        color = col;
        reflect = reflecting;
        eta = refractIndex;
    }
    virtual float intersect(std::string type, const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& intersectPos, glm::vec3& normal) {
        glm::vec3 oc = rayOrigin - center;
        auto a = glm::dot(rayDir, rayDir);
        auto b = 2.0 * glm::dot(oc, rayDir);
        auto c = glm::dot(oc, oc) - radius*radius;
        auto discriminant = b*b - 4*a*c;
        if (type == "Shadow") {
            float t = fmax((-b + pow(b*b - 4*a*c, 0.5))/(2*a),(-b - pow(b*b - 4*a*c, 0.5))/(2*a) );
                if (t > 0) {
                    return 1;
                }
                else {
                    return 0;
                }
            }
        if (discriminant >= 0) {
            float t = std::min((-b + pow(b*b - 4*a*c, 0.5))/(2*a),(-b - pow(b*b - 4*a*c, 0.5))/(2*a) );
            intersectPos = rayOrigin + t*rayDir;
            normal = glm::normalize(intersectPos - center);
            return t;
        } else {
            return -1;
        }
    }
private:
    glm::vec3(color);
    glm::vec3(center);
    double radius;
    bool reflect;
    bool refract;
    float eta;
};

class Plane:public Object {
public:
    Plane(glm::vec3 normal, glm::vec3 point, glm::vec3 col, bool reflecting, float refractIndex, float ambientFactor = 0.2f, float specExponent = 50.0f): Object(col, reflecting, refractIndex, ambientFactor = 0.2f, specExponent = 50.0f) {
        n = normal;
        p = point;
        color = col;
        reflect = reflecting;
        eta = refractIndex;
    }
    virtual float intersect(std::string type, const glm::vec3& rayOrigin, const glm::vec3& rayDir, glm::vec3& intersectPos, glm::vec3& normal) {
        glm::vec3 po = p - rayOrigin;
        float denom = dot(n, rayDir);
        if (abs(denom) > 0) {
            float t= dot(po,n)/denom;
            if (t >= 0) {
                normal = n;
                intersectPos = rayOrigin + t*rayDir;
                return t;
            } else {
                return -1;
            }
        } else {
            return -1;
        }

    }
private:
    glm::vec3(n);
    glm::vec3(p);
    glm::vec3(color);
    bool reflect;
    bool refract;
    float eta;
};
#endif
