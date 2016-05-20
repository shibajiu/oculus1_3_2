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

//using namespace OVR;

int init();
static void error_callback(int error, const char* description);
static void key_callback(GLFWwindow* window1, int key, int scancode, int action, int mods);

static ovrResult result;
static ovrSession session;
static ovrGraphicsLuid luid;
static ovrHmdDesc desc;
static ovrSizei resolution;
static ovrTrackingState ts;
static ovrPoseStatef pose;
static GLFWwindow *window;