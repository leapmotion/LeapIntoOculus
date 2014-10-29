#ifdef __APPLE__
    #define OVR_OS_MAC
    #include <OpenGL/gl.h>
    #include <OpenGL/OpenGL.h>
#elif _WIN32
  #define OVR_OS_WIN
  #define GLEW_STATIC
  #include <GL/glew.h>
  #include "windows.h"
#else
  #include <GL/glew.h>
#endif

#include "Log.h"
#include <SDL2/SDL.h>
#include "Leap.h"
#include "LeapMath.h"
#include "util.h"
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

#ifndef __APP_H__
	#define __APP_H__

//shaders
static const std::string PASSTHROUGH_VERTEXT_SHADER = "vertex.vsl";
static const std::string CORRECTING_FRAGMENT_SHADER = "fragment.fsl";

class App {
	private:
		static App instance;

		bool isRunning = true;

		SDL_Window* window = NULL;
		SDL_Renderer* renderer = NULL;
        SDL_GLContext glContext = NULL;

		int windowWidth = 1024;
		int windowHeight = 768;

        Leap::Controller controller;
        int leapImageWidth = 0;
        int leapImageHeight = 0;
        int leapDistortionWidth = 0;
        int leapDistortionHeight = 0;

        ovrHmd hmd;
        ovrSizei eyeres[2];
        ovrEyeRenderDesc eye_rdesc[2];
        ovrGLTexture fbOVRglTexure[2];
        union ovrGLConfig glcfg;
        unsigned int flags, displayCapabilities;

        //render target for oculus
        GLuint fbo;
        GLuint fbTexture, fbDepth;
        int fbWidth, fbHeight, fbTextureWidth, fbTextureHeight;

        GLuint rawLeftTexture = 0;
        GLuint rawRightTexture = 0;
        GLuint distortionLeftTexture = 0;
        GLuint distortionRightTexture = 0;

	private:
		App();
		void onEvent(SDL_Event* event);
		bool init();
		void update();
		void render();
		void cleanup();
        void working_render();

	public:
		int execute(int argc, char* argv[]);

	public:
		static App* getInstance();

};

#endif