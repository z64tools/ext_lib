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
	Vec3f pos;
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

typedef struct ViewContext {
	f32     fovy;
	f32     fovyTarget;
	f32     near;
	f32     far;
	f32     scale;
	f32     aspect;
	MtxF    modelMtx;
	MtxF    viewMtx;
	MtxF    projMtx;
	CamSettings settings;
	Camera* currentCamera;
	Camera  camera[4];
	Vec2s   projectDim;
	struct {
		u8 cameraControl : 1;
		u8 setCamMove    : 1;
		u8 matchDrawDist : 1;
	};
} ViewContext;

void View_Camera_FlyMode(ViewContext* viewCtx, Input* inputCtx);
void View_Camera_OrbitMode(ViewContext* viewCtx, Input* inputCtx);

void View_Init(ViewContext* view, Input* input);
void View_Update(ViewContext* viewCtx, Input* inputCtx);

void View_SetProjectionDimensions(ViewContext* viewCtx, Vec2s* dim);

void View_Raycast(ViewContext* this, Vec2s pos, Rect dispRect, RayLine* dst);

#endif