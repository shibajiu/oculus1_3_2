#include "o4.h"

void main(int argc, char **argv){
	init();
	system("pause");
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

	// Configure Stereo settings.
	recommenedTex0Size = ovr_GetFovTextureSize(session, ovrEye_Left,
		desc.DefaultEyeFov[0], 1.0f);
	recommenedTex1Size = ovr_GetFovTextureSize(session, ovrEye_Right,
		desc.DefaultEyeFov[1], 1.0f);
	ovrSizei bufferSize;
	bufferSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
	bufferSize.h = max(recommenedTex0Size.h, recommenedTex1Size.h);

	//glEnable(GL_FRAMEBUFFER_SRGB);
	ovrSwapTextureSet * pTextureSet = 0;
	//application should call glEnable(GL_FRAMEBUFFER_SRGB) before rendering into these textures.
	if (ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, bufferSize.w, bufferSize.h,
		&pTextureSet) == ovrSuccess)
	{
		// Sample texture access:
		tex = (ovrGLTexture*)&pTextureSet->Textures[0];
		glBindTexture(GL_TEXTURE_2D, tex->OGL.TexId);		
	}
	
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, desc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, desc.DefaultEyeFov[1]);
	hmdToEyeViewOffset[0] = eyeRenderDesc[0].HmdToEyeViewOffset;
	hmdToEyeViewOffset[1] = eyeRenderDesc[1].HmdToEyeViewOffset;

	// Initialize our single full screen Fov layer.
	
	layer.Header.Type = ovrLayerType_EyeFov;
	layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
	layer.ColorTexture[0] = pTextureSet;
	layer.ColorTexture[1] = pTextureSet;
	layer.Fov[0] = eyeRenderDesc[0].Fov;
	layer.Fov[1] = eyeRenderDesc[1].Fov;
	layer.Viewport[0] = Recti(0, 0, bufferSize.w / 2, bufferSize.h);
	layer.Viewport[1] = Recti(bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);
	
	// ld.RenderPose and ld.SensorSampleTime are updated later per frame.
	isVisible = true;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_NORMALIZE);
	float V[] = { 1, 1, 1, 1 };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, V);
	glEnable(GL_COLOR_MATERIAL);

	glClearColor(0.1, 0.1, 0.1, 1);
	chess_tex = gen_chess_tex(1.0, 0.7, 0.4, 0.4, 0.7, 1.0);

}

void rendering_loop(){

	ovrMatrix4f proj;
	float rot_mat[16];

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_FRAMEBUFFER_SRGB);

	// Get both eye poses simultaneously, with IPD offset already included.
	double displayMidpointSeconds = ovr_GetPredictedDisplayTime(session, 0);
	double sensorSampleTime = ovr_GetTimeInSeconds();
	ovrTrackingState hmdState = ovr_GetTrackingState(session, displayMidpointSeconds, ovrTrue);
	ovr_CalcEyePoses(hmdState.HeadPose.ThePose, hmdToEyeViewOffset, layer.RenderPose);
	layer.SensorSampleTime = sensorSampleTime;

	if (isVisible)
	{
		for (int eye = 0; eye < 2; ++eye){
			glViewport(0, 0, eye == 0 ? recommenedTex0Size.w : recommenedTex1Size.w, eye == 0 ? recommenedTex0Size.h:recommenedTex1Size.h);
			proj = ovrMatrix4f_Projection(desc.DefaultEyeFov[eye], 0.5, 500.0, 1);
			glMatrixMode(GL_PROJECTION);
			glLoadTransposeMatrixf(proj.M[0]);
			//pose[eye] = ovr_GetHmdPosePerEye(hmd, eye);
			
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glTranslatef(hmdToEyeViewOffset[eye].x,
				hmdToEyeViewOffset[eye].y,
				hmdToEyeViewOffset[eye].z);
			quat_to_matrix(&layer.RenderPose[eye].Orientation.x, rot_mat);
			glMultMatrixf(rot_mat);
			/* translate the view matrix with the positional tracking */
			glTranslatef(-layer.RenderPose[eye].Position.x, -layer.RenderPose[eye].Position.y, -layer.RenderPose[eye].Position.z);
			/* move the camera to the eye level of the user */
			glTranslatef(0, -ovr_GetFloat(session, OVR_KEY_EYE_HEIGHT, 1.65), 0);
		}
	}
	glDisable(GL_FRAMEBUFFER_SRGB);
	
	// Do distortion rendering, Present and flush/sync

	// Set up positional data.
	ovrViewScaleDesc viewScaleDesc;
	viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
	viewScaleDesc.HmdToEyeViewOffset[0] = hmdToEyeViewOffset[0];
	viewScaleDesc.HmdToEyeViewOffset[1] = hmdToEyeViewOffset[1];

	ovrLayerHeader* layers = &layer.Header;
	ovrResult result = ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1);
	isVisible = (result == ovrSuccess);

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

void genFBO(){
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	
	glGenTextures(1, &fb_texture);
	glBindTexture(GL_TEXTURE_2D, fb_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex->OGL.Header.TextureSize.w, tex->Texture.Header.TextureSize.h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb_texture, 0);

	glGenRenderbuffers(1, &fb_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, fb_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, tex->OGL.Header.TextureSize.w, tex->Texture.Header.TextureSize.h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb_depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "incomplete framebuffer!\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void quat_to_matrix(const float *quat, float *mat)
{
	mat[0] = 1.0 - 2.0 * quat[1] * quat[1] - 2.0 * quat[2] * quat[2];
	mat[4] = 2.0 * quat[0] * quat[1] + 2.0 * quat[3] * quat[2];
	mat[8] = 2.0 * quat[2] * quat[0] - 2.0 * quat[3] * quat[1];
	mat[12] = 0.0f;

	mat[1] = 2.0 * quat[0] * quat[1] - 2.0 * quat[3] * quat[2];
	mat[5] = 1.0 - 2.0 * quat[0] * quat[0] - 2.0 * quat[2] * quat[2];
	mat[9] = 2.0 * quat[1] * quat[2] + 2.0 * quat[3] * quat[0];
	mat[13] = 0.0f;

	mat[2] = 2.0 * quat[2] * quat[0] + 2.0 * quat[3] * quat[1];
	mat[6] = 2.0 * quat[1] * quat[2] - 2.0 * quat[3] * quat[0];
	mat[10] = 1.0 - 2.0 * quat[0] * quat[0] - 2.0 * quat[1] * quat[1];
	mat[14] = 0.0f;

	mat[3] = mat[7] = mat[11] = 0.0f;
	mat[15] = 1.0f;
}

void draw_scene(void)
{
	int i;
	float grey[] = { 0.8, 0.8, 0.8, 1 };
	float col[] = { 0, 0, 0, 1 };
	float lpos[][4] = {
		{ -8, 2, 10, 1 },
		{ 0, 15, 0, 1 }
	};
	float lcol[][4] = {
		{ 0.8, 0.8, 0.8, 1 },
		{ 0.4, 0.3, 0.3, 1 }
	};

	for (i = 0; i<2; i++) {
		glLightfv(GL_LIGHT0 + i, GL_POSITION, lpos[i]);
		glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, lcol[i]);
	}

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glTranslatef(0, 10, 0);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, grey);
	glBindTexture(GL_TEXTURE_2D, chess_tex);
	glEnable(GL_TEXTURE_2D);
	draw_box(30, 20, 30, -1.0);
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();

	for (i = 0; i<4; i++) {
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, grey);
		glPushMatrix();
		glTranslatef(i & 1 ? 5 : -5, 1, i & 2 ? -5 : 5);
		draw_box(0.5, 2, 0.5, 1.0);
		glPopMatrix();

		col[0] = i & 1 ? 1.0 : 0.3;
		col[1] = i == 0 ? 1.0 : 0.3;
		col[2] = i & 2 ? 1.0 : 0.3;
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, col);

		glPushMatrix();
		if (i & 1) {
			glTranslatef(0, 0.25, i & 2 ? 2 : -2);
		}
		else {
			glTranslatef(i & 2 ? 2 : -2, 0.25, 0);
		}
		draw_box(0.5, 0.5, 0.5, 1.0);
		glPopMatrix();
	}

	col[0] = 1;
	col[1] = 1;
	col[2] = 0.4;
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, col);
	draw_box(0.05, 1.2, 6, 1.0);
	draw_box(6, 1.2, 0.05, 1.0);
}

void draw_box(float xsz, float ysz, float zsz, float norm_sign)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(xsz * 0.5, ysz * 0.5, zsz * 0.5);

	if (norm_sign < 0.0) {
		glFrontFace(GL_CW);
	}

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1 * norm_sign);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, 1);
	glTexCoord2f(1, 0); glVertex3f(1, -1, 1);
	glTexCoord2f(1, 1); glVertex3f(1, 1, 1);
	glTexCoord2f(0, 1); glVertex3f(-1, 1, 1);
	glNormal3f(1 * norm_sign, 0, 0);
	glTexCoord2f(0, 0); glVertex3f(1, -1, 1);
	glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
	glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
	glTexCoord2f(0, 1); glVertex3f(1, 1, 1);
	glNormal3f(0, 0, -1 * norm_sign);
	glTexCoord2f(0, 0); glVertex3f(1, -1, -1);
	glTexCoord2f(1, 0); glVertex3f(-1, -1, -1);
	glTexCoord2f(1, 1); glVertex3f(-1, 1, -1);
	glTexCoord2f(0, 1); glVertex3f(1, 1, -1);
	glNormal3f(-1 * norm_sign, 0, 0);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	glTexCoord2f(1, 0); glVertex3f(-1, -1, 1);
	glTexCoord2f(1, 1); glVertex3f(-1, 1, 1);
	glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0, 1 * norm_sign, 0);
	glTexCoord2f(0.5, 0.5); glVertex3f(0, 1, 0);
	glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
	glTexCoord2f(1, 0); glVertex3f(1, 1, 1);
	glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
	glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
	glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0, -1 * norm_sign, 0);
	glTexCoord2f(0.5, 0.5); glVertex3f(0, -1, 0);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
	glTexCoord2f(1, 1); glVertex3f(1, -1, 1);
	glTexCoord2f(0, 1); glVertex3f(-1, -1, 1);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	glEnd();

	glFrontFace(GL_CCW);
	glPopMatrix();
}

unsigned int gen_chess_tex(float r0, float g0, float b0, float r1, float g1, float b1)
{
	int i, j;
	unsigned int tex;
	unsigned char img[8 * 8 * 3];
	unsigned char *pix = img;

	for (i = 0; i<8; i++) {
		for (j = 0; j<8; j++) {
			int black = (i & 1) == (j & 1);
			pix[0] = (black ? r0 : r1) * 255;
			pix[1] = (black ? g0 : g1) * 255;
			pix[2] = (black ? b0 : b1) * 255;
			pix += 3;
		}
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, img);

	return tex;
}
