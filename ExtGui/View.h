#ifndef __Z64VIEW_H__
#define __Z64VIEW_H__
#include <ExtLib.h>
#include <ExtGui/Math.h>
#include <ExtGui/Matrix.h>
#include <ExtGui/Input.h>
#include <ExtGui/Collision.h>

typedef struct {
	Vec3f eye;
	Vec3f at;
	Vec3f up;
	
	Vec3f vel;
	Vec3f offset;
	f32   dist;
	f32   targetDist;
	s16   pitch;
	s16   yaw;
	s16   roll;
	
	// s16   vYaw;
	// s16   vPitch;
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
	f32   targetStep;
	Vec3f targetPos;
	u32   moveToTarget;
	
	f32  fovy;
	f32  fovyTarget;
	f32  near;
	f32  far;
	f32  scale;
	f32  aspect;
	MtxF modelMtx;
	MtxF viewMtx;
	MtxF projMtx;
	CamSettings settings;
	Camera*     currentCamera;
	Camera      camera[4];
	Vec2s       projectDim;
	CamMode     mode;
	struct {
		u8 cameraControl : 1;
		u8 setCamMove    : 1;
	};
} View3D;

void View_Camera_FlyMode(View3D* viewCtx, Input* inputCtx);
void View_Camera_OrbitMode(View3D* viewCtx, Input* inputCtx);

void View_Init(View3D* view, Input* input);
void View_Update(View3D* viewCtx, Input* inputCtx);

void View_SetProjectionDimensions(View3D* viewCtx, Vec2s* dim);

RayLine View_Raycast(View3D* this, Vec2s pos, Rect dispRect);
void View_MoveTo(View3D* this, Vec3f pos);

#endif