#include "o4.h"

void main(int argc, char **argv){

}

int init(){
	//LibOVR needs to be initialized before GLFW
	result = ovr_Initialize(nullptr);
	if (!OVR_SUCCESS(result)){
		fprintf(stderr, "Failed to initialize libOVR.\n");
		return 0;
	}

	//virtual HMD could be enabled through RiftConfigUtil
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result)){
		fprintf(stderr, "Failed to create the HMD.\n");
		ovr_Shutdown();
		return 0;
	}

	desc = ovr_GetHmdDesc(session);
	resolution = desc.Resolution;
	float frustomHorizontalFOV = desc.CameraFrustumHFovInRadians;

	// Query the HMD for ts current tracking state.
	ts = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), ovrTrue);

	//includes full six degrees of freedom (6DoF) head tracking data including orientation, position, and their first and second derivatives.
	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)){
		pose = ts.HeadPose;
	}

#pragma region glfw
	if (!glfwInit()){
		fprintf(stderr, "Failed to initialize glfw.\n");
		return 0;
	}
	//create a glfw window
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	if (!(window = glfwCreateWindow(640, 480, "Oculus", NULL, NULL))){
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
#pragma endregion initialize glfw
}

void shutdowm(){
	// LibOVR must be shut down after GLFW.
	glfwTerminate();
	ovr_Destroy(session);
	ovr_Shutdown();
}

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

static void key_callback(GLFWwindow* window1, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window1, GL_TRUE);
}