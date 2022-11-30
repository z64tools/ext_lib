#include <glad/glad.h>
#include "ext_interface.h"

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/src/nanovg_gl.h>

bool gTick;
const f64 gNativeFPS = 60;
ThreadLocal bool gLimitFPS = true;
ThreadLocal static bool sPrevLimitFPS;
ThreadLocal static void* sWindow;

void* GET_LOCAL_WINDOW(void) {
    return sWindow;
}

AppInfo* GET_APPINFO(void* window) {
    return glfwGetWindowUserPointer(window);
}

void* GET_CONTEXT(void* window) {
    return GET_APPINFO(window)->context;
}

Input* GET_INPUT(void* window) {
    return GET_APPINFO(window)->input;
}

static void Interface_Update(AppInfo* app) {
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
    
    app->state &= ~APP_RESIZE_CALLBACK;
    Input_End(app->input);
}

static void Interface_ManageMsgWindow(AppInfo* app) {
    SubWindow* child = app->subWinHead;
    SubWindow* next;
    
    while (child) {
        next = child->next;
        
        if (child->app.state & APP_CLOSED) {
            glfwDestroyWindow(child->app.window);
            Node_Kill(app->subWinHead, child);
        }
        
        child = next;
    }
}

static void Interface_FramebufferCallback(GLFWwindow* window, s32 width, s32 height) {
    AppInfo* app = GET_APPINFO(window);
    
    glViewport(0, 0, width, height);
    
    app->prevWinDim.x = app->wdim.x;
    app->prevWinDim.y = app->wdim.y;
    app->wdim.x = width;
    app->wdim.y = height;
    app->state |= APP_RESIZE_CALLBACK;
    
    Interface_Update(app);
    glfwSwapBuffers(app->window);
}

void* Interface_Init(const char* title, AppInfo* app, Input* input, void* context, CallbackFunc updateCall, CallbackFunc drawCall, DropCallback dropCallback, u32 x, u32 y, u32 samples) {
    app->context = context;
    app->input = input;
    app->updateCall = updateCall;
    app->drawCall = drawCall;
    app->wdim.x = x;
    app->wdim.y = y;
    app->state |= APP_MAIN;
    
    Log("glfw Init");
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    printf_info(glfwGetVersionString());
    
    if (samples)
        glfwWindowHint(GLFW_SAMPLES, samples);
    
    Log("Create Window");
    sWindow = app->window = glfwCreateWindow(app->wdim.x, app->wdim.y, title, NULL, NULL);
    
    if (app->window == NULL)
        printf_error("Failed to create GLFW window.");
    
    glfwSetWindowUserPointer(app->window, app);
    Log("Set Callbacks");
    glfwMakeContextCurrent(app->window);
    glfwSetFramebufferSizeCallback(app->window, Interface_FramebufferCallback);
    glfwSetMouseButtonCallback(app->window, InputCallback_Mouse);
    glfwSetCursorPosCallback(app->window, InputCallback_MousePos);
    glfwSetKeyCallback(app->window, InputCallback_Key);
    glfwSetCharCallback(app->window, InputCallback_Text);
    glfwSetScrollCallback(app->window, InputCallback_Scroll);
    glfwSetWindowFocusCallback(app->window, NULL);
    glfwSwapInterval(gLimitFPS);
    
    if (dropCallback)
        glfwSetDropCallback(app->window, (void*)dropCallback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        printf_error("Failed to initialize GLAD.");
    
    Log("Init Matrix, Input and set Framerate");
    Matrix_Init();
    Input_Init(input, app);
    
    Log("Done!");
    
    return nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
}

void Interface_Main(AppInfo* app) {
    static f32 tickMod;
    
    while (!glfwWindowShouldClose(app->window)) {
        Profiler_I(0xF0);
        Time_Start(0xF0);
        
        glfwPollEvents();
        Interface_Update(app);
        if (app->state & APP_MAIN)
            Interface_ManageMsgWindow(app);
        
        Profiler_O(0xF0);
        
        glfwSwapBuffers(app->window);
        if (sPrevLimitFPS != gLimitFPS)
            glfwSwapInterval(gLimitFPS);
        sPrevLimitFPS = gLimitFPS;
        
        gDeltaTime = Time_Get(0xF0) / (1.0 / gNativeFPS);
        
        tickMod += gDeltaTime;
        if (tickMod >= 1.0f) {
            tickMod = WrapF(tickMod, 0.0f, 1.0f);
            gTick = true;
        } else gTick = false;
    }
    
    app->state |= APP_CLOSED;
    printf_info("App Closed");
}

void Interface_Destroy(AppInfo* app) {
    glfwDestroyWindow(app->window);
    glfwTerminate();
}

static void MessageWindow_Update(MessageWindow* this) {
}

static void MessageWindow_Draw(MessageWindow* this) {
    void* vg = this->window.vg;
    
    glViewport(0, 0, UnfoldVec2(this->window.app.wdim));
    nvgBeginFrame(vg, UnfoldVec2(this->window.app.wdim), 1.0f);
    
    nvgBeginPath(vg);
    nvgFillColor(vg, Theme_GetColor(THEME_BASE, 255, 1.0f));
    nvgRect(vg, 0, 0, UnfoldVec2(this->window.app.wdim));
    nvgFill(vg);
    
    nvgEndFrame(vg);
}

static void MessageWindow_Thread(MessageWindow* this) {
    glfwMakeContextCurrent(this->window.app.window);
    Interface_Main(&this->window.app);
}

SubWindow* Interface_MessageWindow(AppInfo* parentApp, const char* title, const char* message) {
    MessageWindow* this = Calloc(sizeof(*this));
    Thread thd;
    
    {
        s32 x, y;
        AppInfo* app = &this->window.app;
        app->context = this;
        app->input = &this->window.input;
        app->updateCall = (void*)MessageWindow_Update;
        app->drawCall = (void*)MessageWindow_Draw;
        app->wdim.x = 1920 * 0.2;
        app->wdim.y = 1080 * 0.15;
        app->state |= APP_MSG_WIN;
        
        glfwGetWindowPos(parentApp->window, &x, &y);
        
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, false);
        glfwWindowHint(GLFW_ICONIFIED, false);
        
        app->window = glfwCreateWindow(app->wdim.x, app->wdim.y, title, NULL, NULL);
        
        if (app->window == NULL)
            printf_error("Failed to create GLFW window.");
        
        glfwSetWindowPos(
            app->window,
            x + parentApp->wdim.x * 0.5 - app->wdim.x * 0.5,
            y + parentApp->wdim.y * 0.5 - app->wdim.y * 0.5
        );
        
        glfwSetWindowUserPointer(app->window, app);
        Log("Set Callbacks");
        glfwSetFramebufferSizeCallback(app->window, Interface_FramebufferCallback);
        glfwSetMouseButtonCallback(app->window, InputCallback_Mouse);
        glfwSetCursorPosCallback(app->window, InputCallback_MousePos);
        glfwSetKeyCallback(app->window, InputCallback_Key);
        glfwSetCharCallback(app->window, InputCallback_Text);
        glfwSetScrollCallback(app->window, InputCallback_Scroll);
        
        glfwMakeContextCurrent(app->window);
        glfwSwapInterval(gLimitFPS);
        
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            printf_error("Failed to initialize GLAD.");
        
        Log("Init Matrix, Input and set Framerate");
        Matrix_Init();
        Input_Init(&this->window.input, app);
        
        Log("Done!");
        
        this->window.vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    }
    
    Thread_Create(&thd, MessageWindow_Thread, this);
    glfwMakeContextCurrent(parentApp->window);
    Node_Add(parentApp->subWinHead, (&this->window));
    
    return &this->window;
}
