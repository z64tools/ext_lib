#ifndef EXT_VIEW_H
#define EXT_VIEW_H
#include "ext_lib.h"
#include "ext_math.h"
#include "ext_matrix.h"
#include "ext_input.h"
#include "ext_collision.h"

typedef struct {
	Vec3f eye;
	Vec3f at;
	Vec3f up;
	Vec3f right;
	
	f32 fovy;
	f32 fovyTarget;
	
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
	CAM_MODE_FLY   = (1 << 0),
	CAM_MODE_ORBIT = (1 << 1),
	
	CAM_MODE_ALL   = (CAM_MODE_FLY | CAM_MODE_ORBIT),
} CamMode;

typedef Rect (*ViewGetRectFunc)(void*);
typedef Vec2f (*ViewGetPointFunc)(void*);

typedef struct View3D {
	f32  near;
	f32  far;
	f32  scale;
	f32  aspect;
	MtxF modelMtx;
	MtxF viewMtx;
	MtxF projMtx;
	MtxF projViewMtx;
	CamSettings settings;
	Camera*     currentCamera;
	Camera      camera[4];
	CamMode     mode;
	RayLine     ray;
	struct {
		bool cameraControl : 1;
		bool isControlled  : 1;
		bool usePreCalcRay : 1;
		bool ortho         : 1;
		bool interrupt     : 1;
		bool rotToAngle    : 1;
		bool moveToTarget  : 1;
		bool noSmooth      : 1;
	};
	
	Vec3s targetRot;
	f32   targetStep;
	Vec3f targetPos;
	
	void* pass;
	Rect (*getViewRect)(void*);
	Vec2f (*getCursorPos)(void*);
} View3D;

void View_Camera_FlyMode(View3D* viewCtx, Input* inputCtx);
void View_Camera_OrbitMode(View3D* viewCtx, Input* inputCtx);

void View_Init(View3D* this, Input* inputCtx, void* pass, ViewGetRectFunc getRect, ViewGetPointFunc getPoint);
void View_InterruptControl(View3D* this);
void View_Update(View3D* viewCtx, Input* inputCtx);

bool View_CheckControlKeys(Input* input);
RayLine View_GetRayLine(View3D* this, Vec2f point);
Vec3f View_GetProjectPoint(View3D* this, Vec2f point);
RayLine View_GetCursorRayLine(View3D* this);
MtxF View_GetOrientedMtxF(View3D* this, f32 x, f32 y, f32 z);
MtxF View_GetLockOrientedMtxF(View3D* view, f32 dgr, int axis_id, bool viewFix);
void View_MoveTo(View3D* this, Vec3f pos);
void View_ZoomTo(View3D* this, f32 zoom);
void View_RotTo(View3D* this, Vec3s rot);
Vec3f View_OrientDirToView(View3D* this, Vec3f dir);
Vec2f View_GetLocalScreenPos(View3D* this, Vec3f point);
Vec2f View_GetScreenPos(View3D* this, Vec3f point);
bool View_PointInScreen(View3D* this, Vec3f point);
f32 View_DepthFactor(View3D* this, Vec3f point);
f32 View_DimFactor(View3D* this);

void View_ClipPointIntoView(View3D* this, Vec3f* a, Vec3f normal);

#endif
