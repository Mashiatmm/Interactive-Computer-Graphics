#ifndef RAY_HPP
#define RAY_HPP

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_core_3_3.h"
#include <GL/freeglut.h>

using namespace std;
using namespace glm;

class Ray {
    vec3 origin;
    vec3 dir;

public:
    Ray(vec3 origin, vec3 dir){
        this->origin = origin;
        this->dir = dir;
    }

    vec3 getDir(){
        return dir;
    }

    vec3 getOrigin(){
        return origin;
    }

    void setDir(vec3 dir){
        this->dir = dir;
    }

    void setOrigin(vec3 origin){
        this->origin = origin;
    }

    void printRay(){
        cout<< " Ray\nOrigin: " << origin.x << ", " << origin.y << ", " << origin.z << endl;
        cout<< " Direction: " << dir.x << ", " << dir.y << ", " << dir.z << endl;
    }
	
};

#endif