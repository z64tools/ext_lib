#include <glad/glad.h>
#include <nanovg/src/nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/src/nanovg_gl.h>
#include "ext_interface.h"
#include <ext_texel.h>

f32 gPixelRatio = 1.0f;
f32 gPixelScale = 1.0f;
const f64 gNativeFPS = 60;

extern DataFile gHack;
extern DataFile gHackBold;
void (*gUiInitFunc)();
void (*gUiNanoFunc)(void*);
void (*gUiDestFunc)();

static void Update(Window* this) {
	if (this->updateCall == NULL || this->drawCall == NULL) {
		const char* TRUE = PRNT_BLUE "true" PRNT_RSET;
		const char* FALSE = PRNT_REDD "false" PRNT_RSET;
		errr("Window " PRNT_GRAY "[" PRNT_YELW "%s" PRNT_GRAY "]" PRNT_RSET "\nUpdate: %s\nDraw: %s", this->title, this->updateCall != NULL ? TRUE : FALSE, this->drawCall != NULL ? TRUE : FALSE);
	}
	
	if (!glfwGetWindowAttrib(this->glfw, GLFW_ICONIFIED)) {
		s32 winWidth, winHeight;
		s32 fbWidth, fbHeight;
		
		glfwGetWindowSize(this->glfw, &winWidth, &winHeight);
		glfwGetFramebufferSize(this->glfw, &fbWidth, &fbHeight);
		
		gPixelRatio = (float)fbWidth / (float)winWidth;
		
		Input_Update(this->input);
		this->updateCall(this->context);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glViewport(0, 0, winWidth, winHeight);
		this->drawCall(this->context);
		glfwSwapBuffers(this->glfw);
		
		Input_End(this->input);
	}
	
	this->state &= ~APP_RESIZE_CALLBACK;
}

static void Window_FramebufferCallback(GLFWwindow* glfw, s32 width, s32 height) {
	Window* this = GET_WINDOW();
	
	this->state |= APP_RESIZE_CALLBACK;
	
	this->prevDim.x = this->dim.x;
	this->prevDim.y = this->dim.y;
	this->dim.x = width;
	this->dim.y = height;
	Update(this);
}

GLFWwindow* GET_GLFWWINDOW(void) {
	return glfwGetCurrentContext();
}

Window* GET_WINDOW() {
	return glfwGetWindowUserPointer(GET_GLFWWINDOW());
}

void* GET_CONTEXT() {
	return GET_WINDOW()->context;
}

Input* GET_INPUT() {
	return GET_WINDOW()->input;
}

void glViewportRect(s32 x, s32 y, s32 w, s32 h) {
	Window* this = GET_WINDOW();
	
	glViewport(x, this->dim.y - y - h, w, h);
}

static void* AllocNanoVG() {
	void* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	
	osLog("Register Fonts");
	nvgCreateFontMem(vg, "default", gHackBold.data, gHackBold.size, 0);
	
	if (gUiNanoFunc)
		gUiNanoFunc(vg);
	
	return vg;
}

void* Window_Init(const char* title, Window* this, Input* input, void* context, WindowFunction updateCall, WindowFunction drawCall, DropCallback dropCallback, u32 x, u32 y, u32 samples) {
	this->context = context;
	this->input = input;
	this->updateCall = updateCall;
	this->drawCall = drawCall;
	this->dim.x = x;
	this->dim.y = y;
	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	if (gUiInitFunc)
		gUiInitFunc();
	
	if (samples)
		glfwWindowHint(GLFW_SAMPLES, samples);
	
	osLog("Create Window");
	this->glfw = glfwCreateWindow(this->dim.x, this->dim.y, title, NULL, NULL);
	strncpy(this->title, title, 64);
	
	if (this->glfw == NULL)
		errr("Failed to create GLFW window.");
	
	glfwSetWindowUserPointer(this->glfw, this);
	osLog("Set Callbacks");
	glfwMakeContextCurrent(this->glfw);
	glfwSetFramebufferSizeCallback(this->glfw, Window_FramebufferCallback);
	glfwSetMouseButtonCallback(this->glfw, InputCallback_Mouse);
	glfwSetCursorPosCallback(this->glfw, InputCallback_MousePos);
	glfwSetKeyCallback(this->glfw, InputCallback_Key);
	glfwSetCharCallback(this->glfw, InputCallback_Text);
	glfwSetScrollCallback(this->glfw, InputCallback_Scroll);
	glfwSetWindowFocusCallback(this->glfw, NULL);
	glfwSwapInterval(1);
	
	if (dropCallback)
		glfwSetDropCallback(this->glfw, (void*)dropCallback);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		errr("Failed to initialize GLAD.");
	
	osLog("Init Matrix, Input and set Framerate");
	Matrix_Init();
	Input_Init(input, this);
	glfwSwapInterval(1);
	
	glfwSetWindowPos(this->glfw,
		1920 / 2 - x / 2,
		1080 / 2 - y / 2);
	
	osLog("Done!");
	
	f32 sWidth, sHeight;
	
	glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &sWidth, &sHeight);
	gPixelScale = (sWidth + sHeight) / 2.0f;
	
	osAssert(gPixelScale > EPSILON);
	
	return this->vg = AllocNanoVG();
}

void Window_Close(Window* this) {
	glfwSetWindowShouldClose(this->glfw, GLFW_TRUE);
}

void Window_Update(Window* this) {
	while (!(this->state & APP_CLOSED)) {
		if (!glfwWindowShouldClose(this->glfw)) {
			glfwMakeContextCurrent(this->glfw);
			time_start(0xF0);
			this->private.tickMod += gDeltaTime;
			if (this->private.tickMod >= 1.0f) {
				this->private.tickMod = wrapf(this->private.tickMod, 0.0f, 1.0f);
				this->tick = true;
			} else this->tick = false;
			
			Update(this);
			glfwPollEvents();
			gDeltaTime = time_get(0xF0) / (1.0 / gNativeFPS);
		} else {
			osLog("Close Window: [%s]", this->title);
			this->state |= APP_CLOSED;
			glfwDestroyWindow(this->glfw);
			nvgDeleteGL3(this->vg);
			
			this->glfw = this->vg = NULL;
		}
	}
	
	osLog("OK");
	glfwTerminate();
	
	if (gUiDestFunc)
		gUiDestFunc();
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3native.h>

#else

#endif

#undef Window_SetParam
void Window_SetParam(Window* this, u32 num, ...) {
	va_list va;
	GLFWwindow* original = glfwGetCurrentContext();
	
	glfwMakeContextCurrent(this->glfw);
	
	va_start(va, num);
	osAssert(num % 2 == 0);
	
	for (var_t i = 0; i < num; i += 2) {
		WindowParam flag = va_arg(va, WindowParam);
		int value = va_arg(va, int);
		
#ifdef _WIN32
		HWND win = glfwGetWin32Window(this->glfw);
		
		switch (flag) {
			case WIN_MAXIMIZE:
				if (value)
					SetWindowLongPtr(win, GWL_STYLE, GetWindowLongPtrA(win, GWL_STYLE) | WS_MAXIMIZEBOX);
				else
					SetWindowLongPtr(win, GWL_STYLE, GetWindowLongPtrA(win, GWL_STYLE) & ~(WS_MAXIMIZEBOX));
				break;
			case WIN_MINIMIZE:
				if (value)
					SetWindowLongPtr(win, GWL_STYLE, GetWindowLongPtrA(win, GWL_STYLE) | WS_MINIMIZEBOX);
				else
					SetWindowLongPtr(win, GWL_STYLE, GetWindowLongPtrA(win, GWL_STYLE) & ~(WS_MINIMIZEBOX));
				break;
		}
		
#else
		switch (flag) {
			case WIN_MAXIMIZE:
				break;
				
			case WIN_MINIMIZE:
				break;
		}
		
		value++;
#endif
	}
	
	va_end(va);
	
	glfwMakeContextCurrent(original);
}

////////////////////////////////////////////////////////////////////////////////
