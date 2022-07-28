#include "Interface.h"

static void Camera_CalculateFly(Camera* cam) {
	Vec3f zero = Math_Vec3f_New(0, 0, 0);
	Vec3f upOffset = Math_Vec3f_New(0, 1, 0);
	Vec3f atOffset = Math_Vec3f_New(0, 0, cam->dist);
	
	Matrix_Push();
	Matrix_Translate(cam->eye.x, cam->eye.y, cam->eye.z, MTXMODE_NEW);
	Matrix_RotateY_s(cam->yaw, MTXMODE_APPLY);
	Matrix_RotateX_s(cam->pitch, MTXMODE_APPLY);
	Matrix_RotateZ_s(cam->roll, MTXMODE_APPLY);
	
	Matrix_Translate(cam->vel.x, cam->vel.y, cam->vel.z, MTXMODE_APPLY);
	Matrix_MultVec3f(&zero, &cam->eye);
	
	Matrix_MultVec3f(&upOffset, &cam->up);
	cam->up = Math_Vec3f_Sub(cam->up, cam->eye);
	Matrix_MultVec3f(&atOffset, &cam->at);
	
	Matrix_Pop();
}

void View_Camera_FlyMode(ViewContext* this, Input* inputCtx) {
	Camera* cam = this->currentCamera;
	s16 pitch = Math_Vec3f_Pitch(cam->eye, cam->at);
	static s32 flip;
	static s32 init;
	
	if (this->cameraControl) {
		if (inputCtx->key[KEY_LEFT_SHIFT].hold)
			Math_DelSmoothStepToF(&cam->speed, cam->speedMod * 4, 0.25f, 1.00f, 0.00001f);
		
		else if (inputCtx->key[KEY_SPACE].hold)
			Math_DelSmoothStepToF(&cam->speed, cam->speedMod * 8, 0.25f, 1.00f, 0.00001f);
		
		else
			Math_DelSmoothStepToF(&cam->speed, cam->speedMod, 0.25f, 1.00f, 0.00001f);
		
		if (inputCtx->key[KEY_A].hold || inputCtx->key[KEY_D].hold) {
			if (inputCtx->key[KEY_A].hold)
				Math_DelSmoothStepToF(&cam->vel.x, cam->speed, 0.25f, 1.00f, 0.00001f);
			if (inputCtx->key[KEY_D].hold)
				Math_DelSmoothStepToF(&cam->vel.x, -cam->speed, 0.25f, 1.00f, 0.00001f);
		} else {
			Math_DelSmoothStepToF(&cam->vel.x, 0, 0.25f, 1.00f, 0.00001f);
		}
		
		if (inputCtx->key[KEY_W].hold || inputCtx->key[KEY_S].hold) {
			if (inputCtx->key[KEY_W].hold)
				Math_DelSmoothStepToF(&cam->vel.z, cam->speed, 0.25f, 1.00f, 0.00001f);
			if (inputCtx->key[KEY_S].hold)
				Math_DelSmoothStepToF(&cam->vel.z, -cam->speed, 0.25f, 1.00f, 0.00001f);
		} else {
			Math_DelSmoothStepToF(&cam->vel.z, 0, 0.25f, 1.00f, 0.00001f);
		}
		
		if (Input_GetMouse(inputCtx, MOUSE_L)->hold) {
			if (init == 0)
				flip = Abs(cam->pitch - pitch) < DegToBin(0.1) ? 1 : -1;
			init = 1;
			cam->pitch = (s32)(cam->pitch + inputCtx->mouse.vel.y * 55.5f);
			cam->yaw = (s32)(cam->yaw - inputCtx->mouse.vel.x * 55.5f * flip);
		} else
			init = 0;
	} else {
		init = 0;
		Math_DelSmoothStepToF(&cam->speed, 0.5f, 0.25f, 1.00f, 0.00001f);
		Math_DelSmoothStepToF(&cam->vel.x, 0, 0.25f, 1.00f, 0.00001f);
		Math_DelSmoothStepToF(&cam->vel.z, 0, 0.25f, 1.00f, 0.00001f);
		Math_DelSmoothStepToF(&cam->vel.y, 0, 0.25f, 1.00f, 0.00001f);
	}
	
	Camera_CalculateFly(cam);
}

static void Camera_CalculateOrbit(Camera* cam) {
	Vec3f zero = Math_Vec3f_New(0, 0, 0);
	Vec3f upOffset = Math_Vec3f_New(0, 1, 0);
	Vec3f atOffset = Math_Vec3f_New(0, 0, -cam->dist);
	
	Matrix_Push();
	Matrix_Translate(cam->at.x, cam->at.y, cam->at.z, MTXMODE_NEW);
	Matrix_RotateY_s(cam->yaw, MTXMODE_APPLY);
	Matrix_RotateX_s(cam->pitch, MTXMODE_APPLY);
	Matrix_RotateZ_s(cam->roll, MTXMODE_APPLY);
	
	Matrix_Translate(cam->offset.x, cam->offset.y, cam->offset.z, MTXMODE_APPLY);
	Matrix_MultVec3f(&zero, &cam->at);
	
	Matrix_MultVec3f(&upOffset, &cam->up);
	cam->up = Math_Vec3f_Sub(cam->up, cam->at);
	Matrix_MultVec3f(&atOffset, &cam->eye);
	
	Matrix_Pop();
}

void View_Camera_OrbitMode(ViewContext* this, Input* inputCtx) {
	Camera* cam = this->currentCamera;
	f32 distMult = (cam->dist * 0.1);
	f32 disdiff = fabsf(cam->dist - cam->targetDist);
	
	cam->offset.y = 0;
	
	cam->offset.x = 0;
	if (this->cameraControl) {
		if (inputCtx->mouse.clickMid.hold || inputCtx->mouse.scrollY || disdiff > 1.0) {
			if (inputCtx->mouse.scrollY || disdiff > 1.0f) {
				cam->targetDist -= inputCtx->mouse.scrollY * distMult;
				cam->targetDist = ClampMin(cam->targetDist, 1.0f);
				Math_DelSmoothStepToF(&cam->dist, cam->targetDist, 0.25f, 5.0f * distMult, 0.01f * distMult);
			}
			
			if (inputCtx->mouse.clickMid.hold) {
				if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
					cam->offset.y = inputCtx->mouse.vel.y * distMult * 0.01f;
					cam->offset.x = inputCtx->mouse.vel.x * distMult * 0.01f;
				} else {
					cam->yaw -= inputCtx->mouse.vel.x * 67;
					cam->pitch += inputCtx->mouse.vel.y * 67;
				}
			}
			if (inputCtx->key[KEY_LEFT_CONTROL].hold) {
				this->fovyTarget = Clamp(this->fovyTarget * (1.0 + (inputCtx->mouse.scrollY / 20)), 30, 120);
			} else {
				Camera_CalculateOrbit(cam);
			}
		}
	}
}

void View_Init(ViewContext* this, Input* inputCtx) {
	Camera* cam;
	
	this->currentCamera = &this->camera[0];
	cam = this->currentCamera;
	
	cam->pos = Math_Vec3f_New(0, 0, -150.0f);
	cam->dist = cam->targetDist = 300.f;
	cam->speedMod = 5.0f;
	
	Camera_CalculateFly(cam);
	Matrix_LookAt(&this->viewMtx, cam->eye, cam->at, cam->up);
	
	this->fovyTarget = this->fovy = 65;
	this->near = 10.0;
	this->far = 12800.0;
	this->scale = 1;
}

void View_Update(ViewContext* this, Input* inputCtx) {
	Camera* cam = this->currentCamera;
	Vec3f vdir;
	f32 fovDiff = -this->fovy;
	
	Math_DelSmoothStepToF(&this->fovy, this->fovyTarget, 0.25, 5.25f, 0.00001);
	fovDiff += this->fovy;
	
	if (fovDiff != 0) {
		vdir = Math_Vec3f_Median(cam->eye, cam->at);
		vdir = Math_Vec3f_Sub(vdir, cam->eye);
		vdir = Math_Vec3f_Normalize(vdir);
		vdir = Math_Vec3f_MulVal(vdir, fovDiff * ((120 / this->fovy) - 1.0f) * 2.5f);
		cam->eye = Math_Vec3f_Add(cam->eye, vdir);
		cam->at = Math_Vec3f_Add(cam->at, vdir);
	}
	
	Matrix_Projection(
		&this->projMtx,
		this->fovy,
		(f32)this->projectDim.x / (f32)this->projectDim.y,
		this->near,
		this->far,
		this->scale
	);
	
	if (inputCtx->mouse.click.press || inputCtx->mouse.scrollY)
		this->setCamMove = 1;
	
	if (inputCtx->mouse.cursorAction == 0)
		this->setCamMove = 0;
	
	View_Camera_OrbitMode(this, inputCtx);
	View_Camera_FlyMode(this, inputCtx);
	
	Matrix_LookAt(&this->viewMtx, cam->eye, cam->at, cam->up);
	
	Matrix_Scale(1.0, 1.0, 1.0, MTXMODE_NEW);
	Matrix_ToMtxF(&this->modelMtx);
}

void View_SetProjectionDimensions(ViewContext* this, Vec2s* dim) {
	this->projectDim = *dim;
}
