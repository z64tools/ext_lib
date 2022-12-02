#ifndef EXT_VIEW_H
#define EXT_VIEW_H
#include "ext_lib.h"
#include "ext_math.h"
#include "ext_matrix.h"
#include "ext_input.h"
#include "ext_collision.h"

struct Split;

typedef struct {
    Vec3f eye;
    Vec3f at;
    Vec3f up;
    Vec3f right;
    
    Vec3f vel;
    Vec3f offset;
    f32   dist;
    f32   targetDist;
    s16   pitch;
    s16   yaw;
    s16   roll;
    
    f32 speed;
    f32 speedMod;
} Camera;

typedef struct {
    u8 smoothZoom : 1;
} CamSettings;

typedef enum {
    CAM_MODE_FLY,
    CAM_MODE_ORBIT,
    CAM_MODE_ALL
} CamMode;

typedef struct View3D {
    f32  fovy;
    f32  fovyTarget;
    f32  near;
    f32  far;
    f32  scale;
    f32  aspect;
    MtxF modelMtx;
    MtxF viewMtx;
    MtxF projMtx;
    MtxF projViewMtx;
    CamSettings   settings;
    Camera*       currentCamera;
    Camera        camera[4];
    CamMode       mode;
    RayLine       ray;
    struct Split* split;
    struct {
        bool cameraControl : 1;
        bool isControlled  : 1;
        bool usePreCalcRay : 1;
        bool ortho         : 1;
        bool interrupt     : 1;
        bool rotToAngle    : 1;
        bool moveToTarget  : 1;
    };
    
    Vec3s targetRot;
    f32   targetStep;
    Vec3f targetPos;
} View3D;

void View_Camera_FlyMode(View3D* viewCtx, Input* inputCtx);
void View_Camera_OrbitMode(View3D* viewCtx, Input* inputCtx);

void View_Init(View3D* view, Input* input);
void View_InterruptControl(View3D* this);
void View_Update(View3D* viewCtx, Input* inputCtx, struct Split* split);

bool View_CheckControlKeys(Input* input);
RayLine View_GetPointRayLine(View3D* this, Vec2f point);
RayLine View_GetCursorRayLine(View3D* this);
void View_MoveTo(View3D* this, Vec3f pos);
void View_ZoomTo(View3D* this, f32 zoom);
void View_RotTo(View3D* this, Vec3s rot);
Vec3f View_OrientDirToView(View3D* this, Vec3f dir);
Vec2f View_GetScreenPos(View3D* this, Vec3f point);
bool View_PointInScreen(View3D* this, Vec3f point);
f32 View_DepthFactor(View3D* this, Vec3f point);
f32 View_DimFactor(View3D* this);

void View_ClipPointIntoView(View3D* this, Vec3f* a, Vec3f normal);

#endif