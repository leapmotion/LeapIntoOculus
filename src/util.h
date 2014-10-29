//
//  util.h
//  VRHello
//
//  Created by Joe Ward on 10/23/14.
//  Copyright (c) 2014 Leap Motion, Inc. All rights reserved.
//

#ifndef __VRHello__util__
#define __VRHello__util__

#include <stdio.h>
#include <OpenGL/gl.h>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <SDL2/SDL.h>
#include "LeapMath.h"


GLuint createProgram(std::string vertex, std::string fragment);
GLuint createTextureReference();
void printProgramInfoLog(GLuint obj);
void printShaderInfoLog(GLuint obj);
std::string getFileContents(std::string filename);
GLfloat* createPerspectiveMatrix(float fov, float aspect, float near, float far);
void setPerspectiveFrustrum(GLdouble fovY, GLdouble aspect, GLdouble near, GLdouble far);

#endif /* defined(__VRHello__util__) */
