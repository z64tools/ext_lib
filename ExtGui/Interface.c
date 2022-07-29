#include <glad/glad.h>
#include <ExtGui/Interface.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/src/nanovg_gl.h>

f64 gFPS = 70;

AppInfo* GetAppInfo(void* window) {
	return glfwGetWindowUserPointer(window);
}

void* GetUserCtx(void* window) {
	return GetAppInfo(window)->context;
}

static void Interface_Draw(AppInfo* app) {
	Input_Update(app->input);
	
	if (!glfwGetWindowAttrib(app->window, GLFW_ICONIFIED))
		app->updateCall(app->context);
	
	if (app->drawCall) {
		if (!glfwGetWindowAttrib(app->window, GLFW_ICONIFIED)) {
			s32 winWidth, winHeight;
			s32 fbWidth, fbHeight;
			glfwGetWindowSize(app->window, &winWidth, &winHeight);
			glfwGetFramebufferSize(app->window, &fbWidth, &fbHeight);
			gPixelRatio = (float)fbWidth / (float)winWidth;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			app->drawCall(app->context);
		}
	}
	
	app->isResizeCallback = false;
	Input_End(app->input);
	glfwSwapBuffers(app->window);
}

static void Interface_FramebufferCallback(GLFWwindow* window, s32 width, s32 height) {
	AppInfo* app = GetAppInfo(window);
	
	glViewport(0, 0, width, height);
	
	app->prevWinDim.x = app->winDim.x;
	app->prevWinDim.y = app->winDim.y;
	app->winDim.x = width;
	app->winDim.y = height;
	app->isResizeCallback = true;
	
	Interface_Draw(app);
}

void* Interface_Init(const char* title, AppInfo* app, Input* input, void* context, CallbackFunc updateCall, CallbackFunc drawCall, DropCallback dropCallback, u32 x, u32 y, u32 samples) {
	app->context = context;
	app->input = input;
	app->updateCall = updateCall;
	app->drawCall = drawCall;
	app->winDim.x = x;
	app->winDim.y = y;
	
	Log("glfw Init");
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	printf_info(glfwGetVersionString());
	
	if (samples)
		glfwWindowHint(GLFW_SAMPLES, samples);
	
	Log("Create Window");
	app->window = glfwCreateWindow(
		app->winDim.x,
		app->winDim.y,
		title,
		NULL,
		NULL
	);
	if (app->window == NULL) {
		printf_error("Failed to create GLFW window.");
	}
	
	glfwSetWindowUserPointer(app->window, app);
	Log("Set Callbacks");
	glfwMakeContextCurrent(app->window);
	glfwSetFramebufferSizeCallback(app->window, Interface_FramebufferCallback);
	glfwSetMouseButtonCallback(app->window, InputCallback_Mouse);
	glfwSetCursorPosCallback(app->window, InputCallback_MousePos);
	glfwSetKeyCallback(app->window, InputCallback_Key);
	glfwSetCharCallback(app->window, InputCallback_Text);
	glfwSetScrollCallback(app->window, InputCallback_Scroll);
	glfwSwapInterval(0);
	
	if (dropCallback)
		glfwSetDropCallback(app->window, (void*)dropCallback);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf_error("Failed to initialize GLAD.");
	}
	
	Log("Init Matrix, Input and set Framerate");
	Matrix_Init();
	Input_Init(input, app);
	
	Log("Done!");
	
	return nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
}

void Interface_Main(AppInfo* app) {
	f64 prev = glfwGetTime();
	
	while (!glfwWindowShouldClose(app->window)) {
		Interface_Draw(app);
		glfwPollEvents();
		
		while (glfwGetTime() < prev + 1.0 / gFPS)
			(void)0;
		
		prev += 1.0 / gFPS;
	}
	
	glfwDestroyWindow(app->window);
	glfwTerminate();
}