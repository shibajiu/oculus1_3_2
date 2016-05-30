#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define OVR_OS_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#define OVR_OS_MAC
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#define OVR_OS_LINUX
#endif

#include <stdio.h>
#include<stdlib.h>
#include <GL/glew.h>
#include <GLFW\glfw3.h>
#include <GLFW/glfw3native.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

using namespace OVR;

int init();
void rendering_loop();
void quat_to_matrix(const float *quat, float *mat);
void draw_scene(void);
void draw_box(float xsz, float ysz, float zsz, float norm_sign);
void genFBO();
unsigned int gen_chess_tex(float r0, float g0, float b0, float r1, float g1, float b1);
static void error_callback(int error, const char* description);
static void key_callback(GLFWwindow* window1, int key, int scancode, int action, int mods);
static void scroll_callback(GLFWwindow* window, double x, double y);

static ovrResult result;
static ovrSession session;
static ovrGraphicsLuid luid;
static ovrHmdDesc desc;
static ovrSizei resolution, recommenedTex0Size, recommenedTex1Size;
static ovrSizei bufferSize;
static ovrGLTexture* tex;
static ovrTrackingState ts;
static ovrPoseStatef pose;
static GLFWwindow *window;
static GLuint fbo, fb_depth, fb_texture, chess_tex;
static ovrEyeRenderDesc eyeRenderDesc[2];
static ovrVector3f hmdToEyeViewOffset[2];
static ovrLayerEyeFov layer;
static bool isVisible;
static ovrSwapTextureSet * pTextureSet;