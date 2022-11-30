#ifndef EXT_INTERFACE_H
#define EXT_INTERFACE_H

#include "ext_lib.h"
#include "ext_cursor.h"
#include "ext_input.h"
#include "ext_matrix.h"
#include "ext_view.h"
#include "ext_theme.h"
#include "ext_geogrid.h"
#include "ext_collision.h"

extern bool gTick;
extern const f64 gNativeFPS;
extern ThreadLocal bool gLimitFPS;

struct MessageWindow;

typedef void (*CallbackFunc)(void*);
typedef void (*DropCallback)(GLFWwindow*, s32, char* item[]);

typedef enum {
    APP_RESIZE_CALLBACK = 1 << 0,
    APP_MAIN            = 1 << 1,
    APP_MSG_WIN         = 1 << 2,
    APP_CLOSED          = 1 << 3,
} AppState;

typedef struct AppInfo {
    GLFWwindow*  window;
    CallbackFunc updateCall;
    CallbackFunc drawCall;
    Input*       input;
    void*    context;
    Vec2s    wdim;
    Vec2s    bufDim;
    Vec2s    prevWinDim;
    AppState state;
    struct SubWindow* subWinHead;
} AppInfo;

typedef struct SubWindow {
    AppInfo app;
    Input   input;
    void*   vg;
    struct SubWindow* next;
} SubWindow;

typedef struct MessageWindow {
    SubWindow   window;
    s32         value;
    const char* message;
} MessageWindow;

void* GET_LOCAL_WINDOW(void);
AppInfo* GET_APPINFO(void* window);
void* GET_CONTEXT(void* window);
Input* GET_INPUT(void* window);

void* Interface_Init(
    const char* title,
    AppInfo* app,
    Input* input,
    void* context,
    CallbackFunc
    updateCall,
    CallbackFunc drawCall,
    DropCallback dropCallback,
    u32 x, u32 y,
    u32 samples
);
void Interface_Main(AppInfo* app);
void Interface_Destroy(AppInfo* app);
SubWindow* Interface_MessageWindow(AppInfo* parentApp, const char* title, const char* message);

#define GUI_INITIALIZE(                             \
        mainContext, title,                         \
        width, height, antialias,                   \
        Update, Draw, DropCallback)                 \
                                                    \
    (mainContext)->vg = Interface_Init(             \
        title,                                      \
        &(mainContext)->app, &(mainContext)->input, \
        (mainContext), (void*)Update,               \
        (void*)Draw, DropCallback,                  \
        width, height, antialias                    \
        );

#endif
