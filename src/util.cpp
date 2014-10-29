//
//  util.cpp
//  VRHello
//
//  Created by Joe Ward on 10/23/14.
//  Copyright (c) 2014 Leap Motion, Inc. All rights reserved.
//

#include "util.h"

GLuint createProgram(std::string vertexFile, std::string fragmentFile)
{
    std::string vertex = getFileContents(vertexFile);
    std::string fragment = getFileContents(fragmentFile);

    GLint program = glCreateProgram();

    GLint compileSuccess;

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *vsource = (const GLchar *)vertex.c_str();

    glShaderSource(vertexShader, 1, &vsource, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess == GL_FALSE) {
        printf("Problem compiling vertex shader\n");
        printShaderInfoLog(vertexShader);
    }
    glAttachShader(program, vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *fsource = (const GLchar *)fragment.c_str();

    glShaderSource(fragmentShader, 1, &fsource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess == GL_FALSE) {
        printf("Problem compiling fragment shader\n");
        printShaderInfoLog(fragmentShader);
    }

    glAttachShader(program, fragmentShader);
    
    glLinkProgram(program);
    glGetProgramiv(program, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess == GL_FALSE) {
        printf("Problem linking program\n");
        printProgramInfoLog(vertexShader);
        exit(1);
    }


    return program;
}

GLuint createTextureReference()
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return textureID;
}

void printShaderInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n",infoLog);
        free(infoLog);
    }
}

void printProgramInfoLog(GLuint obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 0)
    {
        infoLog = (char *)malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
	printf("%s\n",infoLog);
        free(infoLog);
    }
}

std::string getFileContents(std::string filename)
{
  std::ifstream in(filename);
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    in.close();
    return(contents);
  }
  else if(errno){
    std::cout << "Error: " << errno << std::endl;
  }
  std::cout << "Couldn't open " << filename << std::endl;
  exit(1);
}

GLfloat* createPerspectiveMatrix(float fov, float aspect,
    float near, float far)
{
    GLfloat* m = new GLfloat[16];

    float angle = (fov / 180.0f) * Leap::PI;
    float f = 1.0f / tan( angle * 0.5f );

    m[0] = f / aspect;
    m[5] = f;
    m[10] = (far + near) / (near - far);
    m[11] = -1.0f;
    m[14] = (2.0f * far*near) / (near - far);

    return m;
}

void setPerspectiveFrustrum(GLdouble fovY, GLdouble aspect, GLdouble near, GLdouble far)
{
    GLdouble fW, fH;
    fH = tan( fovY / 360 * Leap::PI ) * near;
    fW = fH * aspect;
    glFrustum( -fW, fW, -fH, fH, near, far );
}

