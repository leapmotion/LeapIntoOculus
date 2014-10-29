//==============================================================================
#include "App.h"
#include <OpenGL/glu.h>

App App::instance;
GLuint vertShader, fragShader, program;

App::App() {}

using namespace Leap;

bool App::init() {
    //Enable Leap Motion images and optimize controller for HMD use
    controller.setPolicyFlags(static_cast<Controller::PolicyFlag>(Controller::PolicyFlag::POLICY_IMAGES | Controller::PolicyFlag::POLICY_OPTIMIZE_HMD));

    //Oculus init
    ovr_Initialize();

    if(!(hmd = ovrHmd_Create(0))) {
    		fprintf(stderr, "failed to open Oculus HMD, falling back to virtual debug HMD\n");
    		if(!(hmd = ovrHmd_CreateDebug(ovrHmd_DK2))) {
    			fprintf(stderr, "failed to create virtual debug HMD\n");
    			return -1;
    		}
   }
   printf("initialized HMD: %s - %s\n", hmd->Manufacturer, hmd->ProductName);
   windowWidth = hmd->Resolution.w;
   windowHeight = hmd->Resolution.h;

    //SDL setup
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
    	Log("Unable to Init SDL: %s", SDL_GetError());
    	return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    Log("If error setting version: %s", SDL_GetError());

    if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        Log("Unable to Init hinting: %s", SDL_GetError());
    }

    if((window = SDL_CreateWindow(
    	"Leap Motion VR",
    	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    	windowWidth, windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL)
    ) == NULL) {
    	Log("Unable to create SDL Window: %s", SDL_GetError());
    	return false;
    }

    glContext = SDL_GL_CreateContext(window);

    if((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)) == NULL) {
        Log("Unable to create renderer");
        return false;
    }

    SDL_GL_MakeCurrent(window, glContext);

    Log("gl_renderer: %s", glGetString(GL_RENDERER));
    Log("gl_version: %s", glGetString(GL_VERSION));
    Log("glsl_version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    //Oculus OpenGL setup
    ovrHmd_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection | ovrTrackingCap_Position, 0);
    eyeres[0] = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left, hmd->DefaultEyeFov[0], 1.0);
    eyeres[1] = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right, hmd->DefaultEyeFov[1], 1.0);

    fbWidth = eyeres[0].w + eyeres[1].w;
    fbHeight = eyeres[0].h > eyeres[1].h ? eyeres[0].h : eyeres[1].h;

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &fbTexture);
    glGenRenderbuffers(1, &fbDepth);

    glBindTexture(GL_TEXTURE_2D, fbTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    fbTextureWidth = (fbWidth);
    fbTextureHeight = (fbHeight);

    glBindTexture(GL_TEXTURE_2D, fbTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbTextureWidth, fbTextureHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbTexture, 0);

    /* create and attach the renderbuffer that will serve as our z-buffer */
    glBindRenderbuffer(GL_RENDERBUFFER, fbDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbTextureWidth, fbTextureHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbDepth);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "incomplete framebuffer!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    printf("created render target: %dx%d (texture size: %dx%d)\n", fbWidth, fbHeight, fbTextureWidth, fbTextureHeight);

 	/* fill in the ovrGLTexture structures that describe our render target texture */
 	for(int i=0; i<2; i++) {
 		fbOVRglTexure[i].OGL.Header.API = ovrRenderAPI_OpenGL;
 		fbOVRglTexure[i].OGL.Header.TextureSize.w = fbTextureWidth;
 		fbOVRglTexure[i].OGL.Header.TextureSize.h = fbTextureHeight;
 		/* this next field is the only one that differs between the two eyes */
 		fbOVRglTexure[i].OGL.Header.RenderViewport.Pos.x = i == 0 ? 0 : fbWidth / 2.0;
 		fbOVRglTexure[i].OGL.Header.RenderViewport.Pos.y = fbTextureHeight - fbHeight;
 		fbOVRglTexure[i].OGL.Header.RenderViewport.Size.w = fbWidth / 2.0;
 		fbOVRglTexure[i].OGL.Header.RenderViewport.Size.h = fbHeight;
 		fbOVRglTexure[i].OGL.TexId = fbTexture;	/* both eyes will use the same texture id */
 	}

 	memset(&glcfg, 0, sizeof glcfg);
 	glcfg.OGL.Header.API = ovrRenderAPI_OpenGL;
 	glcfg.OGL.Header.RTSize = hmd->Resolution;
 	glcfg.OGL.Header.Multisample = 1;

 	ovrHmd_SetEnabledCaps(hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);
 	displayCapabilities = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp |
 		ovrDistortionCap_Overdrive;
 	if(!ovrHmd_ConfigureRendering(hmd, &glcfg.Config, displayCapabilities, hmd->DefaultEyeFov, eye_rdesc)) {
 		fprintf(stderr, "failed to configure distortion renderer\n");
 	}

    //Scene setup

    //Create the shader program
    program = createProgram(PASSTHROUGH_VERTEXT_SHADER, CORRECTING_FRAGMENT_SHADER);
    glUseProgram(program);
    GLuint rawSampler = glGetUniformLocation(program, "rawTexture");
    GLuint distortionSampler  = glGetUniformLocation(program, "distortionTexture");
    glUniform1i(rawSampler, 0);
    glUniform1i(distortionSampler,  1);

    //Create Leap image textures
    rawLeftTexture = createTextureReference();
    rawRightTexture = createTextureReference();
    distortionLeftTexture = createTextureReference();
    distortionRightTexture = createTextureReference();

    return true;
}

int App::execute(int argc, char* argv[]) {
	if(!init()) return 0;
	SDL_Event event;

	while(isRunning) {
		while(SDL_PollEvent(&event) != 0) { onEvent(&event); }
		update();
		render();
		SDL_Delay(1);
	}
	cleanup();
	return 0;
}

void App::update() {
    Frame frame = controller.frame();
    if (frame.isValid()) {
        Image left = frame.images()[0];
        Image right = frame.images()[1];

        //Check images sizes
        if (left.width() != leapImageWidth ||
            left.height() != leapImageHeight ||
            left.distortionWidth() != leapDistortionWidth ||
            left.distortionHeight() != leapDistortionHeight) {
                glBindTexture(GL_TEXTURE_2D, rawLeftTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, left.width(), left.height(), 0, GL_RED, GL_UNSIGNED_BYTE, 0);
                glBindTexture(GL_TEXTURE_2D, rawRightTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, right.width(), right.height(), 0, GL_RED, GL_UNSIGNED_BYTE, 0);
                glBindTexture(GL_TEXTURE_2D, distortionLeftTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, left.distortionWidth()/2, left.distortionHeight(), 0, GL_RG, GL_FLOAT, 0);
                glBindTexture(GL_TEXTURE_2D, distortionRightTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, right.distortionWidth()/2, right.distortionHeight(), 0, GL_RG, GL_FLOAT, 0);

                leapImageWidth = left.width();
                leapImageHeight = left.height();
                leapDistortionWidth = left.distortionWidth();
                leapDistortionHeight = left.distortionHeight();
                std::cout << "Leap Camera Size: " << leapImageWidth << ", " << leapImageHeight << std::endl;
        }

        //Update image and distortion textures
        if (left.width() > 0) {
                glBindTexture(GL_TEXTURE_2D, rawLeftTexture);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, left.width(), left.height(), GL_RED, GL_UNSIGNED_BYTE, left.data());
                glBindTexture(GL_TEXTURE_2D, distortionLeftTexture);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, left.distortionWidth()/2, left.distortionHeight(), GL_RG, GL_FLOAT, left.distortion());
        }
        if (right.width() > 0) {
                glBindTexture(GL_TEXTURE_2D, rawRightTexture);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, right.width(), right.height(), GL_RED, GL_UNSIGNED_BYTE, right.data());
                glBindTexture(GL_TEXTURE_2D, distortionRightTexture);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, right.distortionWidth()/2, right.distortionHeight(), GL_RG, GL_FLOAT, right.distortion());
        }
    }

}

void App::render() {
	ovrMatrix4f proj;

    ovrPosef eyeRenderPoses[2];
    ovrTrackingState hmdState;
    ovrVector3f hmdToEyeViewOffset[2] = { eye_rdesc[0].HmdToEyeViewOffset, eye_rdesc[1].HmdToEyeViewOffset };
    ovrHmd_GetEyePoses(hmd, 0, hmdToEyeViewOffset, eyeRenderPoses, &hmdState);

    ovrHmd_BeginFrame(hmd, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for(int i=0; i<2; i++) {
 		ovrEyeType eye = hmd->EyeRenderOrder[i];
 		glViewport(eye == ovrEye_Left ? 0 : fbWidth / 2, 0, fbWidth / 2, fbHeight);
 		proj = ovrMatrix4f_Projection(hmd->DefaultEyeFov[eye], 0.5, 500.0, 1);
 		glMatrixMode(GL_PROJECTION);
 		glLoadTransposeMatrixf(proj.M[0]);

 		glMatrixMode(GL_MODELVIEW);
 		glLoadIdentity();
        //place sensor quads relative to eye so they have a size-to-distance ratio of 4:1
        glScalef(4, 4, 1.0);
        glTranslatef(0.0, 0.0, -1.0);

        glUseProgram(program);
        if(i != 0){
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rawLeftTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, distortionLeftTexture);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rawRightTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, distortionRightTexture);
        }
        glBegin(GL_QUADS); // Draw A Quad for camera image
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 1.0f, 0.0f); // Top Left
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 1.0f, 0.0f); // Top Right
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,-1.0f, 0.0f); // Bottom Right
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,-1.0f, 0.0f); // Bottom Left
        glEnd(); // Done Drawing The Quad

    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); //Done rendering to fbo
 	glViewport(0, 0, windowWidth, windowHeight);
    ovrHmd_EndFrame(hmd, eyeRenderPoses,  &fbOVRglTexure[0].Texture);
}

void App::onEvent(SDL_Event* event) {
    if(event->type == SDL_QUIT) isRunning = false;
    if(event->type == SDL_KEYDOWN){
        ovrHmd_DismissHSWDisplay(hmd);
    }
}

void App::cleanup() {

	if(hmd) {
 		ovrHmd_Destroy(hmd);
 	}
 	ovr_Shutdown();

	if(renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}

	if(window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}

    SDL_Quit();
}

App* App::getInstance() { return &App::instance; }
