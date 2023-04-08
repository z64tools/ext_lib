#include <glad/glad.h>
#include <nanovg/src/nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/src/nanovg_gl.h>
#include "ext_interface.h"

const f64 gNativeFPS = 60;

static void Interface_Update(AppInfo* app);
static void Interface_Update_AppInfo(AppInfo* app);
static void Interface_Update_SubWindows(AppInfo* app);

typedef enum {
    SUB_WIN_CUSTOM,
    SUB_WIN_MESSAGE_BOX,
    SUB_WIN_FILE_DIALOG,
} SubWindowType;

static void Interface_FramebufferCallback(GLFWwindow* window, s32 width, s32 height) {
    AppInfo* app = GET_APPINFO(window);
    GLFWwindow* restoreWindow = GET_LOCAL_WINDOW();
    
    glfwMakeContextCurrent(app->window);
    
    app->prevWinDim.x = app->wdim.x;
    app->prevWinDim.y = app->wdim.y;
    app->wdim.x = width;
    app->wdim.y = height;
    app->state |= APP_RESIZE_CALLBACK;
    
    Interface_Update(app);
    glfwSwapBuffers(app->window);
    glfwMakeContextCurrent(restoreWindow);
}

void* GET_LOCAL_WINDOW(void) {
    return glfwGetCurrentContext();
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

void glViewportRect(s32 x, s32 y, s32 w, s32 h) {
    AppInfo* app = GET_APPINFO(GET_LOCAL_WINDOW());
    
    glViewport(x, app->wdim.y - y - h, w, h);
}

static void* Interface_AllocVG() {
    void* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    
    extern DataFile gVera;
    
    _log("Register Fonts");
    nvgCreateFontMem(vg, "default", gVera.data, gVera.size, 0);
    nvgCreateFontMem(vg, "default-bold", gVera.data, gVera.size, 0);
    
    return vg;
}

void* Interface_Init(const char* title, AppInfo* app, Input* input, void* context, WindowFunction updateCall, WindowFunction drawCall, DropCallback dropCallback, u32 x, u32 y, u32 samples) {
    app->context = context;
    app->input = input;
    app->updateCall = updateCall;
    app->drawCall = drawCall;
    app->wdim.x = x;
    app->wdim.y = y;
    
    _log("glfw Init");
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    if (samples)
        glfwWindowHint(GLFW_SAMPLES, samples);
    
    _log("Create AppInfo");
    app->window = glfwCreateWindow(app->wdim.x, app->wdim.y, title, NULL, NULL);
    strncpy(app->title, title, 64);
    
    if (app->window == NULL)
        errr("Failed to create GLFW window.");
    
    glfwSetWindowUserPointer(app->window, app);
    _log("Set Callbacks");
    glfwMakeContextCurrent(app->window);
    glfwSetFramebufferSizeCallback(app->window, Interface_FramebufferCallback);
    glfwSetMouseButtonCallback(app->window, InputCallback_Mouse);
    glfwSetCursorPosCallback(app->window, InputCallback_MousePos);
    glfwSetKeyCallback(app->window, InputCallback_Key);
    glfwSetCharCallback(app->window, InputCallback_Text);
    glfwSetScrollCallback(app->window, InputCallback_Scroll);
    glfwSetWindowFocusCallback(app->window, NULL);
    glfwSwapInterval(1);
    
    if (dropCallback)
        glfwSetDropCallback(app->window, (void*)dropCallback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        errr("Failed to initialize GLAD.");
    
    _log("Init Matrix, Input and set Framerate");
    Matrix_Init();
    Input_Init(input, app);
    glfwSwapInterval(1);
    
    _log("Done!");
    
    return app->vg = Interface_AllocVG();
}

static void Interface_Update(AppInfo* app) {
    if (app->updateCall == NULL || app->drawCall == NULL) {
        const char* TRUE = PRNT_BLUE "true" PRNT_RSET;
        const char* FALSE = PRNT_REDD "false" PRNT_RSET;
        errr("Window " PRNT_GRAY "[" PRNT_YELW "%s" PRNT_GRAY "]" PRNT_RSET "\nUpdate: %s\nDraw: %s", app->title, app->updateCall != NULL ? TRUE : FALSE, app->drawCall != NULL ? TRUE : FALSE);
    }
    
    if (!glfwGetWindowAttrib(app->window, GLFW_ICONIFIED)) {
        s32 winWidth, winHeight;
        s32 fbWidth, fbHeight;
        glfwGetWindowSize(app->window, &winWidth, &winHeight);
        glfwGetFramebufferSize(app->window, &fbWidth, &fbHeight);
        gPixelRatio = (float)fbWidth / (float)winWidth;
        
        Input_Update(app->input);
        app->updateCall(app->context);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glViewport(0, 0, winWidth, winHeight);
        app->drawCall(app->context);
        glfwSwapBuffers(app->window);
        
        Input_End(app->input);
    }
    
    app->state &= ~APP_RESIZE_CALLBACK;
}

static void Interface_Update_AppInfo(AppInfo* app) {
    glfwMakeContextCurrent(app->window);
    
    if (!glfwWindowShouldClose(app->window)) {
        app->private.tickMod += gDeltaTime;
        if (app->private.tickMod >= 1.0f) {
            app->private.tickMod = wrapf(app->private.tickMod, 0.0f, 1.0f);
            app->tick = true;
        } else app->tick = false;
        
        Interface_Update(app);
    } else {
        _log("Close Window: [%s]", app->title);
        app->state |= APP_CLOSED;
        glfwDestroyWindow(app->window);
        nvgDeleteGL3(app->vg);
        
        app->window = app->vg = NULL;
    }
}

static void Interface_Update_SubWindows(AppInfo* app) {
    SubWindow* this = app->subWinHead;
    
    while (this != NULL) {
#if 0
        if (!(this->parent->state & APP_CLOSED)) {
            if (glfwGetWindowAttrib(this->parent->window, GLFW_ICONIFIED) != glfwGetWindowAttrib(this->app.window, GLFW_ICONIFIED)) {
                
                if (glfwGetWindowAttrib(this->parent->window, GLFW_ICONIFIED))
                    glfwIconifyWindow(this->app.window);
                else
                    glfwRestoreWindow(this->app.window);
                glfwShowWindow(this->parent->window);
            }
        }
#endif
        
        Interface_Update_AppInfo(&this->app);
        this = this->next;
    }
    
    this = app->subWinHead;
    
    while (this != NULL) {
        if (this->app.state & APP_CLOSED) {
            _log("Kill Window: [%s]", this->app.title);
            
            Node_Remove(app->subWinHead, this);
            if (!this->settings.noDestroy) {
                if (this->destroyCall)
                    this->destroyCall(this->app.context);
                vfree(this->app.context);
            }
        }
        
        this = this->next;
    }
}

void Interface_Close(AppInfo* app) {
    glfwSetWindowShouldClose(app->window, GLFW_TRUE);
}

void Interface_Main(AppInfo* app) {
    while (!(app->state & APP_CLOSED)) {
        time_start(0xF0);
        
        Interface_Update_SubWindows(app);
        Interface_Update_AppInfo(app);
        
        glfwPollEvents();
        gDeltaTime = time_get(0xF0) / (1.0 / gNativeFPS);
    }
    
    _log("Clear Sub Windows");
    for (SubWindow* sub = app->subWinHead; sub != NULL; sub = sub->next)
        sub->app.state |= APP_CLOSED;
    Interface_Update_SubWindows(app);
    
    _log("OK");
    glfwTerminate();
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

void Interface_CreateSubWindow(SubWindow* window, AppInfo* parentApp, s32 x, s32 y, const char* title) {
    if (!(window->app.window = glfwCreateWindow(x, y, title, NULL, NULL)))
        errr("Failed to create window [%s], [%d, %d]", title, x, y);
    
    glfwMakeContextCurrent(window->app.window);
    glfwSetWindowUserPointer(window->app.window, &window->app);
    glfwSetFramebufferSizeCallback(window->app.window, Interface_FramebufferCallback);
    glfwSetMouseButtonCallback(window->app.window, InputCallback_Mouse);
    glfwSetCursorPosCallback(window->app.window, InputCallback_MousePos);
    glfwSetKeyCallback(window->app.window, InputCallback_Key);
    glfwSetCharCallback(window->app.window, InputCallback_Text);
    glfwSetScrollCallback(window->app.window, InputCallback_Scroll);
    
    glfwSetWindowPos(window->app.window,
        1920 / 2 - x / 2,
        1080 / 2 - y / 2);
    
    Input_Init(&window->input, &window->app);
    Node_Add(parentApp->subWinHead, window);
    
    window->app.wdim = Math_Vec2s_New(x, y);
    window->app.vg = Interface_AllocVG();
    window->parent = parentApp;
    strncpy(window->app.title, title, sizeof(window->app.title));
    
    glfwMakeContextCurrent(parentApp->window);
}

int Interface_Closed(SubWindow* window) {
    return (window->app.state & APP_CLOSED);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3native.h>

#else

#endif

#undef Interface_SetParam
void Interface_SetParam(AppInfo* app, u32 num, ...) {
    va_list va;
    GLFWwindow* original = glfwGetCurrentContext();
    
    glfwMakeContextCurrent(app->window);
    
    va_start(va, num);
    _assert(num % 2 == 0);
    
    for (var i = 0; i < num; i += 2) {
        WinParam flag = va_arg(va, WinParam);
        int value = va_arg(va, int);
        
#ifdef _WIN32
        HWND win = glfwGetWin32Window(app->window);
        
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
