#ifndef EXT_INTERFACE_H
#define EXT_INTERFACE_H

#include "ext_geogrid.h"
#include "ext_lib.h"
#include "ext_cursor.h"
#include "ext_input.h"
#include "ext_matrix.h"
#include "ext_view.h"
#include "ext_theme.h"
#include "ext_collision.h"

typedef enum {
    WIN_MAXIMIZE,
    WIN_MINIMIZE,
} WinParam;

typedef void (*WindowFunction)(void*);
typedef void (*DropCallback)(GLFWwindow*, s32, char* item[]);

typedef enum {
    APP_RESIZE_CALLBACK = 1 << 0,
    APP_CLOSED          = 1 << 3,
} AppState;

typedef struct AppInfo {
    char title[64];
    GLFWwindow*    window;
    WindowFunction updateCall;
    WindowFunction drawCall;
    Input*   input;
    void*    context;
    void*    vg;
    Vec2s    wdim;
    Vec2s    bufDim;
    Vec2s    prevWinDim;
    AppState state;
    struct SubWindow* subWinHead;
    bool tick;
    struct {
        f32 tickMod;
    } private;
} AppInfo;

typedef struct SubWindow {
    AppInfo         app;
    AppInfo*        parent;
    Input           input;
    WindowFunction  destroyCall;
    struct {
        bool noDestroy;
    } settings;
    struct SubWindow* next;
} SubWindow;

void* GET_LOCAL_WINDOW(void);
AppInfo* GET_APPINFO(void* window);
void* GET_CONTEXT(void* window);
Input* GET_INPUT(void* window);

void glViewportRect(s32 x, s32 y, s32 w, s32 h);
void* Interface_Init(
    const char* title,
    AppInfo* app,
    Input* input,
    void* context,
    WindowFunction
    updateCall,
    WindowFunction drawCall,
    DropCallback dropCallback,
    u32 x, u32 y,
    u32 samples
);
void Interface_Close(AppInfo* app);
void Interface_Main(AppInfo* app);
void Interface_SetParam(AppInfo* app, u32 num, ...);

void Interface_CreateSubWindow(SubWindow* window, AppInfo* app, s32 x, s32 y, const char* title);
int Interface_Closed(SubWindow* window);

#define FILE_DIALOG_BUF 256

typedef struct {
    SubWindow window;
    GeoGrid   geo;
    
    char path[FILE_DIALOG_BUF];
    List files;
    List folders;
    
    s32 split;
    s32 selected;
    
    ScrollBar scroll;
    
    struct {
        bool doRename : 1;
    };
    
    struct {
        bool multiSelect : 1;
        bool dispSplit   : 1;
    } settings;
    
    struct {
        const char key[64];
        Split      split;
    } private;
    
    ElTextbox travel;
    ElTextbox search;
    ElButton  backButton;
    ElTextbox rename;
} FileDialog;

void FileDialog_New(FileDialog* this, AppInfo* parentApp, const char* title);
void FileDialog_Free(FileDialog* this);

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
#define Interface_SetParam(app, ...) do {                         \
        PPASSERT((NARGS(__VA_ARGS__) % 2) == 0);                  \
        Interface_SetParam(app, NARGS(__VA_ARGS__), __VA_ARGS__); \
} while (0)

#endif
