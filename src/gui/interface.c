#include <glad/glad.h>
#include "ext_interface.h"

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg/src/nanovg_gl.h>

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
    
    info(glfwGetVersionString());
    
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
            app->private.tickMod = WrapF(app->private.tickMod, 0.0f, 1.0f);
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
                free(this->app.context);
            }
        }
        
        this = this->next;
    }
}

void Interface_Main(AppInfo* app) {
    while (!(app->state & APP_CLOSED)) {
        Timer_Start(0xF0);
        
        Interface_Update_SubWindows(app);
        Interface_Update_AppInfo(app);
        
        gDeltaTime = Timer_End(0xF0) / (1.0 / gNativeFPS);
        glfwPollEvents();
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

typedef struct {
    Vec2s scale;
    s32   split;
    bool  disp_split;
    char  path[FILE_DIALOG_BUF];
} FileDialogConfig;

const s32 sFileMagicValue = -23406137;

static FileDialogConfig FileDialog_LoadConfig(const char* title) {
    FileDialogConfig config;
    
    fs_set("%s", sys_appdata());
    
    strncpy(config.path, sys_appdir(), FILE_DIALOG_BUF);
    config.disp_split = true;
    
    if (sys_stat(fs_item("file_dialog.toml"))) {
        Toml toml = {};
        
        Toml_Load(&toml, fs_item("file_dialog.toml"));
        
        config.scale.x = Toml_GetInt(&toml, "scale[0]");
        config.scale.y = Toml_GetInt(&toml, "scale[1]");
        config.split = Toml_GetInt(&toml, "split");
        
        if (Toml_Var(&toml, "%s.disp_split", x_canitize(title)))
            config.disp_split = Toml_GetBool(&toml, "%s.disp_split", x_canitize(title));
        
        if (Toml_Var(&toml, "%s.path", x_canitize(title)))
            strncpy(config.path, Toml_GetStr(&toml, "%s.path", x_canitize(title)), FILE_DIALOG_BUF);
        
        Toml_Free(&toml);
    } else {
        config.scale.x = 1920 / 2.0f;
        config.scale.y = 1080 / 2.0f;
        config.split = config.scale.x * 0.25f;
    }
    
    return config;
}

static void FileDialog_SaveConfig(FileDialog* this) {
    Toml toml = {};
    
    fs_set("%s", sys_appdata());
    
    if (!sys_stat(fs_item("file_dialog.toml")))
        toml = Toml_New();
    else
        Toml_Load(&toml, fs_item("file_dialog.toml"));
    
    Toml_SetVar(&toml, "scale[0]", "%d", this->window.app.wdim.x);
    Toml_SetVar(&toml, "scale[1]", "%d", this->window.app.wdim.y);
    Toml_SetVar(&toml, "split", "%d", this->split);
    Toml_SetVar(&toml, x_fmt("%s.disp_split", x_canitize(this->window.app.title)), "%s", this->settings.dispSplit ? "true" : "false");
    Toml_SetVar(&toml, x_fmt("%s.path", x_canitize(this->window.app.title)), "\"%s\"", this->path);
    
    Toml_Save(&toml, fs_item("file_dialog.toml"));
    Toml_Free(&toml);
}

static s32 FileDialog_PathRead(FileDialog* this, const char* travel) {
    char buf[sizeof(this->path)];
    s32 ret = 1;
    
    warn("Travel [%s]", travel);
    
    if (travel) {
        strcpy(buf, travel);
        strrep(buf, "\\", "/");
        
        if (!sys_isdir(travel)) {
            ret = 0;
            goto exit;
        }
        
        strcpy(this->path, buf);
        
        if (!strend(this->path, "/"))
            strcat(this->path, "/");
    }
    
    List_FreeItems(&this->files);
    List_FreeItems(&this->folders);
    
    this->scroll = 0;
    this->selected = -1;
    
    List_Walk(&this->folders, this->path, 0, LIST_FOLDERS | LIST_RELATIVE);
    List_Walk(&this->files, this->path, 0, LIST_FILES | LIST_RELATIVE);
    List_Sort(&this->folders);
    List_Sort(&this->files);
    
    forlist(i, this->folders) {
        strrep(this->folders.item[i], "\\", "");
        strrep(this->folders.item[i], "/", "");
    }
    
    this->slot = this->files.num + this->folders.num;
    
    _log("Ok");
    
    strncpy(this->travel.txt, this->path, sizeof(this->travel.txt));
    arrzero(this->search.txt);
    glfwSetWindowTitle(this->window.app.window, x_fmt("%s ─ %s", this->window.app.title, this->path));
    
exit:
    strcpy(this->travel.txt, this->path);
    return ret;
}

static void FileDialog_PathAppend(FileDialog* this, const char* dst) {
    if (dst) {
        char path[sizeof(this->path)];
        strcpy(path, this->path);
        
        if (!strcmp(dst, "..")) {
            var slash = 0;
            
            for (var c = strnlen(path, FILE_DIALOG_BUF); c >= strlen("X:"); c--) {
                if (path[c] == '/') {
                    slash++;
                    
                    if (slash == 2) {
                        path[c + 1] = '\0';
                        break;
                    }
                }
            }
            
        } else
            snprintf(path, FILE_DIALOG_BUF, "%s%s/", this->path, dst);
        
        FileDialog_PathRead(this, path);
    }
}

static void FileDialog_FilePanel_ScrollUpdate(FileDialog* this, Rect r) {
    Input* input = &this->window.input;
    InputType* click = Input_GetMouse(input, CLICK_L);
    const f32 visibleSlots = (f32)r.h / SPLIT_ELEM_Y_PADDING;
    
    if (Rect_PointIntersect(&r, UnfoldVec2(input->cursor.pos)))
        this->scroll -= Input_GetScroll(input);
    this->scrollMax = clamp_min(this->slot - visibleSlots, 0);
    
    if (this->holdSlider) {
        if (click->release)
            this->holdSlider = false;
        else {
            f32 y = input->cursor.pos.y + this->sliderHoldOffset;
            
            this->scroll = ((y - r.y) / (r.h * (this->scrollMax / this->slot))) * (this->slot - visibleSlots);
        }
    }
    
    this->scroll = clamp(this->scroll, 0, this->scrollMax);
    
    Math_SmoothStepToF(&this->vscroll, this->scroll, 0.25f, 500.0f, 0.001f);
}

static s32 FileDialog_FilePanel_ScrollDraw(FileDialog* this, Rect r) {
    const f32 visibleSlots = (f32)r.h / SPLIT_ELEM_Y_PADDING;
    Input* input = &this->window.input;
    InputType* click = Input_GetMouse(input, CLICK_L);
    void* vg = this->window.app.vg;
    Rect slider = Rect_New(
        r.x + r.w - 14,
        Lerp(this->vscroll / this->slot, r.y, r.y + r.h),
        12,
        Lerp((this->vscroll + visibleSlots) / this->slot, r.y, r.y + r.h));
    
    slider.y = clamp(slider.y, r.y, r.y + r.h - 8);
    slider.h = clamp(slider.h - slider.y, 16, r.h);
    
    if (Rect_PointIntersect(&slider, UnfoldVec2(input->cursor.pos))) {
        if (click->press) {
            this->holdSlider = true;
            this->sliderHoldOffset = slider.y - input->cursor.pos.y;
        }
        
        if (!this->holdSlider)
            Theme_SmoothStepToCol(&this->sliderColor, Theme_GetColor(THEME_HIGHLIGHT, 220, 1.0f), 0.5f, 1.0f, 0.01f);
    } else if (!this->holdSlider)
        Theme_SmoothStepToCol(&this->sliderColor, Theme_GetColor(THEME_HIGHLIGHT, 125, 1.0f), 0.5f, 1.0f, 0.01f);
    
    if (this->holdSlider)
        Theme_SmoothStepToCol(&this->sliderColor, Theme_GetColor(THEME_PRIM, 220, 1.0f), 0.5f, 1.0f, 0.01f);
    
    NVGcolor slcol = this->sliderColor;
    
    if (visibleSlots > this->slot) {
        NVGcolor al = slcol;
        f32 remap = Remap(visibleSlots / this->slot, 1.0f, 1.0f + (2.0f / this->slot), 0.0f, 1.0f);
        
        al.a = 0;
        remap = clamp(remap, 0.0f, 1.0f);
        
        slcol = Theme_Mix(remap, slcol, al);
    }
    
    Gfx_DrawRounderRect(vg, slider, slcol);
    
    return !this->holdSlider;
}

static const char* FileDialog_GetSelectedItem(FileDialog* this) {
    const List* list[] = {
        &this->folders,
        &this->files
    };
    bool isFolder = false;
    s32 id = this->selected;
    
    if (id < 0) return NULL;
    
    if (id < this->folders.num)
        isFolder = true;
    else
        id -= this->folders.num;
    
    return list[!isFolder]->item[id];
};

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static void FileDialog_FilePanel(FileDialog* this, Rect r) {
    void* vg = this->window.app.vg;
    Input* input = &this->window.input;
    const List* list[] = {
        &this->folders,
        &this->files
    };
    Rect mainr = r;
    s32 pressSlot = 0;
    InputType* click = Input_GetMouse(input, CLICK_L);
    bool cursorInPanel = Rect_PointIntersect(&mainr, UnfoldVec2(input->cursor.pos));
    Rect selectedRect;
    
    Gfx_DrawRounderRect(vg, mainr, Theme_GetColor(THEME_SPLIT, 255, 1.0f));
    
    FileDialog_FilePanel_ScrollUpdate(this, r);
    
    nvgScissor(vg, UnfoldRect(mainr));
    
    this->slot = 0;
    r.h = SPLIT_ELEM_Y_PADDING;
    r.y -= this->vscroll * SPLIT_ELEM_Y_PADDING;
    
    foreach(i, list) {
        forlist(j, *list[i]) {
            if (IsBetween(r.y, mainr.y - r.h, mainr.y + mainr.h)) {
                const char* item = list[i]->item[j];
                NVGcolor color;
                Rect txr = r;
                
                if (this->search.txt[0])
                    if (!stristr(item, this->search.txt))
                        continue;
                
                if (cursorInPanel) {
                    if (Rect_PointIntersect(&r, UnfoldVec2(input->cursor.pos))) {
                        if (click->press) {
                            pressSlot = (this->slot + 1) * (i == 0 ? -1 : 1);
                            info("Click Slot: %d", pressSlot);
                        }
                    }
                }
                
                if (this->slot != this->selected) {
                    if (this->slot % 2) color = Theme_GetColor(THEME_BASE, 255, 0.85f);
                    else color = Theme_GetColor(THEME_BASE, 255, 0.75f);
                } else color = Theme_Mix(0.75f,
                            Theme_GetColor(THEME_BASE, 255, 1.0f),
                            Theme_GetColor(THEME_PRIM, 255, 1.0f));
                
                Gfx_DrawRounderRect(vg, r, color);
                
                if (i == 0) {
                    txr.x += SPLIT_TEXT_PADDING;
                    
                    nvgBeginPath(vg);
                    nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
                    Gfx_Vector(vg, Math_Vec2f_New(txr.x + 2, txr.y + 2), 1.0f, 0, gAssets.folder);
                    nvgFill(vg);
                    
                    txr.x += 16;
                    txr.w -= 16 * 2;
                }
                
                Gfx_Text(vg, txr, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE, Theme_GetColor(THEME_TEXT, 255, 1.0f), item);
            }
            
            if (this->slot == this->selected)
                selectedRect = r;
            
            this->slot++;
            r.y += SPLIT_ELEM_Y_PADDING;
        }
    }
    
    if (this->doRename) {
        s32 ret;
        
        if ((ret = Element_Textbox(&this->rename))) {
            this->doRename = false;
            
            if (ret > 0) {
                const char* item = FileDialog_GetSelectedItem(this);
                const char* input = x_fmt("%s%s", this->path, item);
                const char* output = x_fmt("%s%s", this->path, this->rename.txt);
                
                if (strcspn(this->rename.txt, "\\/:*?\"<>|") == strlen(this->rename.txt)) {
                    sys_mv(input, output);
                    FileDialog_PathRead(this, NULL);
                } else
                    Sys_Sound("error");
            }
            
            info("%d", ret);
        }
    } else {
        if (this->selected >= 0) {
            if (Input_GetKey(input, KEY_F2)->press) {
                strcpy(this->rename.txt, FileDialog_GetSelectedItem(this));
                this->doRename = true;
                this->rename.element.rect = selectedRect;
                
                Element_SetActiveTextbox(&this->geo, NULL, &this->rename);
            }
        }
    }
    
    if (FileDialog_FilePanel_ScrollDraw(this, mainr)) {
        if (pressSlot) {
            u32 slot = abs(pressSlot) - 1;
            
            if (pressSlot < 0 && click->dual) {
                FileDialog_PathAppend(this, this->folders.item[slot]);
                
                info("Path: [%s]", this->path);
            } else if (click->press) {
                
                this->selected = slot;
            }
        }
    }
    
    nvgResetScissor(vg);
}

static void FileDialog_HeaderPanel(FileDialog* this, Rect r) {
    void* vg = this->window.app.vg;
    
    Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_BASE, 255, 1.25f));
    
    this->backButton.element.rect.y = this->search.element.rect.y = this->travel.element.rect.y = r.y + SPLIT_ELEM_X_PADDING;
    this->backButton.element.rect.h = this->search.element.rect.h = this->travel.element.rect.h = r.h - SPLIT_ELEM_X_PADDING * 2;
    
    this->travel.element.rect.x = r.x + r.h;
    this->travel.element.rect.w = (r.w - SPLIT_ELEM_X_PADDING * 2) - (240);
    this->travel.element.rect.w = clamp_min(this->travel.element.rect.w, 120);
    
    this->search.element.rect.x = this->travel.element.rect.x + this->travel.element.rect.w + SPLIT_ELEM_X_PADDING;
    this->search.element.rect.w = (r.w - SPLIT_ELEM_X_PADDING * 2) - (this->travel.element.rect.w + SPLIT_ELEM_X_PADDING * 5);
    
    this->backButton.element.rect.x = r.x + SPLIT_ELEM_X_PADDING;
    this->backButton.element.rect.w = r.h - SPLIT_ELEM_X_PADDING * 2;
    
    if (Element_Button(&this->backButton))
        FileDialog_PathAppend(this, "..");
    
    if (Element_Textbox(&this->travel)) {
        if (!sys_isdir(this->travel.txt))
            strcpy(this->travel.txt, this->path);
        
        else {
            FileDialog_PathRead(this, this->travel.txt);
        }
    }
    
    Element_Textbox(&this->search);
    if (this->search.modified)
        this->vscroll = this->scroll = 0;
}

void FileDialog_Init(FileDialog* this) {
    this->selected = -1;
    this->files = List_New();
    this->folders = List_New();
    
    GeoGrid_Init(&this->geo, &this->window.app, NULL);
    FileDialog_PathRead(this, NULL);
    
    this->search.clearIcon = true;
    this->backButton.icon = gAssets.arrowParent;
}

void FileDialog_Update(void* arg) {
    FileDialog* this = arg;
    
    this->split = clamp(this->split, 0, 256);
}

void FileDialog_Draw(void* arg) {
    FileDialog* this = arg;
    void* vg = this->window.app.vg;
    Vec2s dim = this->window.app.wdim;
    
    nvgBeginFrame(vg, dim.x, dim.y, gPixelRatio); {
        Rect main = { 0, 0, dim.x, dim.y };
        Rect fpr = {};
        Rect hdr = {};
        
        Gfx_DrawRounderRect(vg, main, Theme_GetColor(THEME_BASE, 255, 1.1f));
        
        hdr.y = main.y;
        hdr.h = SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING * 2;
        
        hdr.x = fpr.x = main.x + this->split;
        hdr.w = fpr.w = main.w - this->split;
        fpr.y = main.y + hdr.h;
        fpr.h = main.h - hdr.h;
        
        GeoGrid_Splitless_Start(&this->geo, main);
        FileDialog_FilePanel(this, fpr);
        FileDialog_HeaderPanel(this, hdr);
        GeoGrid_Splitless_End(&this->geo);
        
        _log("OK");
    } nvgEndFrame(vg);
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

const char sFileDialogKey[64] = "8ACwdYwKKKpV1A3qFIOgFQFJnVPgKh3l1d0ywNNsnKrbZ4yTfUFbFVEXsfB3HCMA";

void Interface_CreateSubWindow(SubWindow* window, AppInfo* app, s32 x, s32 y, const char* title) {
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
    
    Input_Init(&window->input, &window->app);
    Node_Add(app->subWinHead, window);
    
    window->app.wdim = Math_Vec2s_New(x, y);
    window->app.vg = Interface_AllocVG();
    window->parent = app;
    strncpy(window->app.title, title, sizeof(window->app.title));
}

void FileDialog_New(FileDialog* this, AppInfo* app, const char* title) {
    const s32 minX = 1920 / 4, maxX = 1920 / 1.5;
    const s32 minY = 1080 / 4, maxY = 1080 / 1.5;
    FileDialogConfig cfg = FileDialog_LoadConfig(title);
    
    _assert (memcmp(this->private.key, sFileDialogKey, sizeof(sFileDialogKey)));
    typezero(this);
    memcpy((void*)this->private.key, sFileDialogKey, sizeof(sFileDialogKey));
    
    cfg.scale.x = clamp(cfg.scale.x, minX, maxX);
    cfg.scale.y = clamp(cfg.scale.y, minY, maxY);
    
    Interface_CreateSubWindow(&this->window, app, cfg.scale.x, cfg.scale.y, title);
    glfwMakeContextCurrent(this->window.app.window);
    glfwSetWindowSizeLimits(this->window.app.window, minX, minY, maxX, maxY);
    Interface_SetParam(&this->window.app, WIN_MAXIMIZE, false, WIN_MINIMIZE, false);
    
    this->window.settings.noDestroy = true;
    this->window.app.context = this;
    this->window.app.updateCall = FileDialog_Update;
    this->window.app.drawCall = FileDialog_Draw;
    
    this->split = cfg.split;
    this->settings.dispSplit = cfg.disp_split;
    strncpy(this->path, cfg.path, FILE_DIALOG_BUF);
    
    FileDialog_Init(this);
    
    _log("Done");
    
    glfwMakeContextCurrent(app->window);
}

s32 FileDialog_Closed(FileDialog* this) {
    return (this->window.app.state & APP_CLOSED);
}

void FileDialog_Free(FileDialog* this) {
    FileDialog_SaveConfig(this);
    
    List_Free(&this->files);
    List_Free(&this->folders);
    memset((void*)this->private.key, 0, 64);
}

#ifdef _WIN32

#define GLFW_EXPOSE_NATIVE_WIN32
#include <windows.h>
#include <GLFW/glfw3native.h>

#else

#endif

#undef Interface_SetParam
void Interface_SetParam(AppInfo* app, u32 num, ...) {
    va_list va;
    
    va_start(va, num);
    
    _assert(num % 2 == 0);
    num /= 2;
    for (var i = 0; i < num; i++) {
        WinParam flag = va_arg(va, s32);
        s32 value = va_arg(va, s32);
        
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
        
        // Fool the compiler, fun
        value = value * 2;
#endif
    }
    
    va_end(va);
}
