#include "Global.h"

void View_Camera_FlyMode(ViewContext* this, InputContext* inputCtx) {
	Camera* cam = this->currentCamera;
	
	if (this->cameraControl) {
		if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
			Math_DelSmoothStepToF(&this->flyMode.speed, 4.0f, 0.25f, 1.00f, 0.00001f);
		} else if (inputCtx->key[KEY_SPACE].hold) {
			Math_DelSmoothStepToF(&this->flyMode.speed, 16.0f, 0.25f, 1.00f, 0.00001f);
		} else {
			Math_DelSmoothStepToF(&this->flyMode.speed, 0.5f, 0.25f, 1.00f, 0.00001f);
		}
		
		if (inputCtx->key[KEY_A].hold || inputCtx->key[KEY_D].hold) {
			if (inputCtx->key[KEY_A].hold)
				Math_DelSmoothStepToF(&this->flyMode.vel.x, this->flyMode.speed, 0.25f, 1.00f, 0.00001f);
			if (inputCtx->key[KEY_D].hold)
				Math_DelSmoothStepToF(&this->flyMode.vel.x, -this->flyMode.speed, 0.25f, 1.00f, 0.00001f);
		} else {
			Math_DelSmoothStepToF(&this->flyMode.vel.x, 0, 0.25f, 1.00f, 0.00001f);
		}
		
		if (inputCtx->key[KEY_W].hold || inputCtx->key[KEY_S].hold) {
			if (inputCtx->key[KEY_W].hold)
				Math_DelSmoothStepToF(&this->flyMode.vel.z, this->flyMode.speed, 0.25f, 1.00f, 0.00001f);
			if (inputCtx->key[KEY_S].hold)
				Math_DelSmoothStepToF(&this->flyMode.vel.z, -this->flyMode.speed, 0.25f, 1.00f, 0.00001f);
		} else {
			Math_DelSmoothStepToF(&this->flyMode.vel.z, 0, 0.25f, 1.00f, 0.00001f);
		}
	} else {
		Math_DelSmoothStepToF(&this->flyMode.speed, 0.5f, 0.25f, 1.00f, 0.00001f);
		Math_DelSmoothStepToF(&this->flyMode.vel.x, 0, 0.25f, 1.00f, 0.00001f);
		Math_DelSmoothStepToF(&this->flyMode.vel.z, 0, 0.25f, 1.00f, 0.00001f);
	}
	
	Vec3f* eye = &cam->eye;
	Vec3f* at = &cam->at;
	
	if (this->flyMode.vel.z || this->flyMode.vel.x || (this->cameraControl && inputCtx->mouse.clickL.hold)) {
		VecSph camSph = {
			.r = Math_Vec3f_DistXYZ(eye, at),
			.yaw = Math_Vec3f_Yaw(at, eye),
			.pitch = Math_Vec3f_Pitch(at, eye)
		};
		
		if (this->cameraControl && inputCtx->mouse.clickL.hold) {
			camSph.yaw -= inputCtx->mouse.vel.x * 65;
			camSph.pitch -= inputCtx->mouse.vel.y * 65;
		}
		
		*at = *eye;
		
		Math_AddVecSphToVec3f(at, &camSph);
	}
	
	if (this->flyMode.vel.z || this->flyMode.vel.x) {
		VecSph velSph = {
			.r = this->flyMode.vel.z * 1000,
			.yaw = Math_Vec3f_Yaw(at, eye),
			.pitch = Math_Vec3f_Pitch(at, eye)
		};
		
		Math_AddVecSphToVec3f(eye, &velSph);
		Math_AddVecSphToVec3f(at, &velSph);
		
		velSph = (VecSph) {
			.r = this->flyMode.vel.x * 1000,
			.yaw = Math_Vec3f_Yaw(at, eye) + 0x3FFF,
			.pitch = 0
		};
		
		Math_AddVecSphToVec3f(eye, &velSph);
		Math_AddVecSphToVec3f(at, &velSph);
	}
}

void View_Camera_OrbitMode(ViewContext* this, InputContext* inputCtx) {
	Camera* cam = this->currentCamera;
	VecSph orbitSph = {
		.r = Math_Vec3f_DistXYZ(&cam->at, &cam->eye),
		.yaw = Math_Vec3f_Yaw(&cam->eye, &cam->at),
		.pitch = Math_Vec3f_Pitch(&cam->eye, &cam->at)
	};
	f32 distMult = (orbitSph.r * 0.1);
	
	if (this->cameraControl) {
		if (inputCtx->mouse.clickMid.hold || inputCtx->mouse.scrollY) {
			if (inputCtx->mouse.clickMid.hold) {
				if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
					VecSph velSph = {
						.r = inputCtx->mouse.vel.y * distMult * 0.01f,
						.yaw = Math_Vec3f_Yaw(&cam->at, &cam->eye),
						.pitch = Math_Vec3f_Pitch(&cam->at, &cam->eye) + 0x3FFF
					};
					
					Math_AddVecSphToVec3f(&cam->eye, &velSph);
					Math_AddVecSphToVec3f(&cam->at, &velSph);
					
					velSph = (VecSph) {
						.r = inputCtx->mouse.vel.x * distMult * 0.01f,
						.yaw = Math_Vec3f_Yaw(&cam->at, &cam->eye) + 0x3FFF,
						.pitch = 0
					};
					
					Math_AddVecSphToVec3f(&cam->eye, &velSph);
					Math_AddVecSphToVec3f(&cam->at, &velSph);
				} else {
					orbitSph.yaw -= inputCtx->mouse.vel.x * 67;
					orbitSph.pitch += inputCtx->mouse.vel.y * 67;
				}
			}
			if (inputCtx->key[KEY_LEFT_CONTROL].hold) {
				this->fovyTarget = Clamp(this->fovyTarget * (1.0 + (inputCtx->mouse.scrollY / 20)), 20, 170);
			} else {
				orbitSph.r = ClampMin(orbitSph.r - (distMult * (inputCtx->mouse.scrollY)), 0.00001f);
				cam->eye = cam->at;
				
				Math_AddVecSphToVec3f(&cam->eye, &orbitSph);
			}
		}
	}
}

void View_Init(ViewContext* this, InputContext* inputCtx) {
	Camera* cam;
	
	this->currentCamera = &this->camera[0];
	cam = this->currentCamera;
	
	cam->eye = (Vec3f) { -50.0f * 100, 50.0f * 100, 50.0f * 100 };
	cam->at = (Vec3f) { 0, 0, 0 };
	
	cam->eye = (Vec3f) { 0, 0, 50.0f * 100 };
	cam->at = (Vec3f) { 0, 0, 0 };
	cam->roll = 0;
	
	Matrix_LookAt(&this->viewMtx, cam->eye, cam->at, cam->roll);
	
	this->fovyTarget = this->fovy = 65;
	this->near = 10.0;
	this->far = 12800.0;
	this->scale = 1;
}

void View_Update(ViewContext* this, InputContext* inputCtx) {
	Camera* cam = this->currentCamera;
	
	Math_DelSmoothStepToF(&this->fovy, this->fovyTarget, 0.25, 5.25f, 0.00001);
	
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
	Matrix_LookAt(&this->viewMtx, cam->eye, cam->at, cam->roll);
	
	Matrix_Scale(1.0, 1.0, 1.0, MTXMODE_NEW);
	Matrix_ToMtxF(&this->modelMtx);
}

void View_SetProjectionDimensions(ViewContext* this, Vec2s* dim) {
	this->projectDim = *dim;
}
