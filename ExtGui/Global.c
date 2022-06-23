#include <glad/glad.h>
#include <ExtGui/Global.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/src/nanovg_gl.h>

AppInfo* __appInfo;
InputContext* __inputCtx;

static void Interface_Draw() {
	Input_Update(__inputCtx, __appInfo);
	
	if (!glfwGetWindowAttrib(__appInfo->mainWindow, GLFW_ICONIFIED))
		__appInfo->updateCall(__appInfo->context);
	
	if (__appInfo->drawCall) {
		if (!glfwGetWindowAttrib(__appInfo->mainWindow, GLFW_ICONIFIED)) {
			s32 winWidth, winHeight;
			s32 fbWidth, fbHeight;
			glfwGetWindowSize(__appInfo->mainWindow, &winWidth, &winHeight);
			glfwGetFramebufferSize(__appInfo->mainWindow, &fbWidth, &fbHeight);
			gPixelRatio = (float)fbWidth / (float)winWidth;
			
			__appInfo->drawCall(__appInfo->context);
		}
	}
	
	__appInfo->isResizeCallback = false;
	Input_End(__inputCtx);
	glfwSwapBuffers(__appInfo->mainWindow);
}

static void Interface_FramebufferCallback(GLFWwindow* window, s32 width, s32 height) {
	glViewport(0, 0, width, height);
	__appInfo->prevWinDim.x = __appInfo->winDim.x;
	__appInfo->prevWinDim.y = __appInfo->winDim.y;
	__appInfo->winDim.x = width;
	__appInfo->winDim.y = height;
	__appInfo->isResizeCallback = true;
	
	Interface_Draw();
}

void* Interface_Init(const char* title, AppInfo* appInfo, InputContext* inputCtx, void* context, CallbackFunc updateCall, CallbackFunc drawCall, DropCallback dropCallback, u32 x, u32 y, u32 samples) {
	__appInfo = appInfo;
	__inputCtx = inputCtx;
	
	appInfo->context = context;
	appInfo->updateCall = updateCall;
	appInfo->drawCall = drawCall;
	appInfo->winDim.x = x;
	appInfo->winDim.y = y;
	
	Log("glfw Init");
	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	if (samples)
		glfwWindowHint(GLFW_SAMPLES, samples);
	
	Log("Create Window");
	appInfo->mainWindow = glfwCreateWindow(
		appInfo->winDim.x,
		appInfo->winDim.y,
		title,
		NULL,
		NULL
	);
	if (appInfo->mainWindow == NULL) {
		printf_error("Failed to create GLFW window.");
	}
	
	Log("Set Callbacks");
	glfwMakeContextCurrent(appInfo->mainWindow);
	
	glfwSetFramebufferSizeCallback(appInfo->mainWindow, Interface_FramebufferCallback);
	glfwSetMouseButtonCallback(appInfo->mainWindow, Input_MouseClickCallback);
	glfwSetKeyCallback(appInfo->mainWindow, Input_KeyCallback);
	glfwSetCharCallback(appInfo->mainWindow, Input_TextCallback);
	glfwSetScrollCallback(appInfo->mainWindow, Input_ScrollCallback);
	if (dropCallback)
		glfwSetDropCallback(appInfo->mainWindow, (void*)dropCallback);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf_error("Failed to initialize GLAD.");
	}
	
	Log("Init Matrix, Input and set Framerate");
	Matrix_Init();
	Input_Init(inputCtx);
	
	Log("Done!");
	
	return nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
}

void Interface_Main() {
	while (!glfwWindowShouldClose(__appInfo->mainWindow)) {
		glfwPollEvents();
		Interface_Draw();
	}
}