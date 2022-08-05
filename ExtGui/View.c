#include "Interface.h"

static int glhUnProjectf(float winx, float winy, float winz, float* modelview, float* projection, int* viewport, float* objectCoordinate);

static void Camera_CalculateFly(Camera* cam) {
	Vec3f zero = Math_Vec3f_New(0, 0, 0);
	Vec3f upOffset = Math_Vec3f_New(0, 1, 0);
	Vec3f atOffset = Math_Vec3f_New(0, 0, cam->dist);
	
	Matrix_Push();
	
	// Math_DelSmoothStepToS(&cam->vYaw, cam->yaw, 7, DegToBin(45), DegToBin(0.1));
	// Math_DelSmoothStepToS(&cam->vPitch, cam->pitch, 7, DegToBin(45), DegToBin(0.1));
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

void View_Camera_FlyMode(View3D* this, Input* inputCtx) {
	Camera* cam = this->currentCamera;
	s16 pitch = Math_Vec3f_Pitch(cam->eye, cam->at);
	static s32 flip;
	static s32 init;
	
	if (this->cameraControl) {
		f32 step = 1.0f * cam->speedMod;
		f32 min = 0.01f * cam->speedMod;
		
		if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
			step *= 4;
			Math_DelSmoothStepToF(&cam->speed, cam->speedMod * 4 * gDeltaTime, 0.25f, 1.00f, 0.1f);
		} else if (inputCtx->key[KEY_SPACE].hold) {
			step *= 8;
			Math_DelSmoothStepToF(&cam->speed, cam->speedMod * 8 * gDeltaTime, 0.25f, 1.00f, 0.1f);
		} else {
			Math_DelSmoothStepToF(&cam->speed, cam->speedMod * gDeltaTime, 0.25f, 1.00f, 0.1f);
		}
		
		if (inputCtx->key[KEY_A].hold || inputCtx->key[KEY_D].hold) {
			if (inputCtx->key[KEY_A].hold)
				Math_DelSmoothStepToF(&cam->vel.x, cam->speed, 0.25f, step, min);
			if (inputCtx->key[KEY_D].hold)
				Math_DelSmoothStepToF(&cam->vel.x, -cam->speed, 0.25f, step, min);
		} else {
			Math_DelSmoothStepToF(&cam->vel.x, 0, 0.25f, step, min);
		}
		
		if (inputCtx->key[KEY_W].hold || inputCtx->key[KEY_S].hold) {
			if (inputCtx->key[KEY_W].hold)
				Math_DelSmoothStepToF(&cam->vel.z, cam->speed, 0.25f, step, min);
			if (inputCtx->key[KEY_S].hold)
				Math_DelSmoothStepToF(&cam->vel.z, -cam->speed, 0.25f, step, min);
		} else {
			Math_DelSmoothStepToF(&cam->vel.z, 0, 0.25f, step, min);
		}
		
		if (inputCtx->key[KEY_Q].hold || inputCtx->key[KEY_E].hold) {
			if (inputCtx->key[KEY_Q].hold)
				Math_DelSmoothStepToF(&cam->vel.y, -cam->speed, 0.25f, step, min);
			if (inputCtx->key[KEY_E].hold)
				Math_DelSmoothStepToF(&cam->vel.y, cam->speed, 0.25f, step, min);
		} else {
			Math_DelSmoothStepToF(&cam->vel.y, 0, 0.25f, step, min);
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
		Math_DelSmoothStepToF(&cam->speed, 0.5f, 0.5f, 1.00f, 0.1f);
		Math_DelSmoothStepToF(&cam->vel.x, 0.0f, 0.5f, 1.00f, 0.1f);
		Math_DelSmoothStepToF(&cam->vel.z, 0.0f, 0.5f, 1.00f, 0.1f);
		Math_DelSmoothStepToF(&cam->vel.y, 0.0f, 0.5f, 1.00f, 0.1f);
	}
	
	Camera_CalculateFly(cam);
}

static void Camera_CalculateOrbit(Camera* cam) {
	Vec3f zero = Math_Vec3f_New(0, 0, 0);
	Vec3f upOffset = Math_Vec3f_New(0, 1, 0);
	Vec3f atOffset = Math_Vec3f_New(0, 0, -cam->dist);
	
	Matrix_Push();
	Matrix_Translate(cam->at.x, cam->at.y, cam->at.z, MTXMODE_NEW);
	
	// cam->vYaw = cam->yaw;
	// cam->vPitch = cam->pitch;
	Matrix_RotateY_s(cam->yaw, MTXMODE_APPLY);
	Matrix_RotateX_s(cam->pitch, MTXMODE_APPLY);
	Matrix_RotateZ_s(cam->roll, MTXMODE_APPLY);
	
	Matrix_Translate(cam->offset.x, cam->offset.y, cam->offset.z, MTXMODE_APPLY);
	cam->offset = Math_Vec3f_New(0, 0, 0);
	Matrix_MultVec3f(&zero, &cam->at);
	
	Matrix_MultVec3f(&upOffset, &cam->up);
	cam->up = Math_Vec3f_Sub(cam->up, cam->at);
	Matrix_MultVec3f(&atOffset, &cam->eye);
	
	Matrix_Pop();
}

static void Camera_CalculateMove(Camera* cam, f32 x, f32 y, f32 z) {
	Vec3f zero = Math_Vec3f_New(0, 0, 0);
	Vec3f upOffset = Math_Vec3f_New(0, 1, 0);
	Vec3f atOffset = Math_Vec3f_New(0, 0, -cam->dist);
	
	Matrix_Push();
	Matrix_Translate(cam->at.x + x, cam->at.y + y, cam->at.z + z, MTXMODE_NEW);
	
	// cam->vYaw = cam->yaw;
	// cam->vPitch = cam->pitch;
	Matrix_RotateY_s(cam->yaw, MTXMODE_APPLY);
	Matrix_RotateX_s(cam->pitch, MTXMODE_APPLY);
	Matrix_RotateZ_s(cam->roll, MTXMODE_APPLY);
	
	Matrix_Translate(cam->offset.x, cam->offset.y, cam->offset.z, MTXMODE_APPLY);
	cam->offset = Math_Vec3f_New(0, 0, 0);
	Matrix_MultVec3f(&zero, &cam->at);
	
	Matrix_MultVec3f(&upOffset, &cam->up);
	cam->up = Math_Vec3f_Sub(cam->up, cam->at);
	Matrix_MultVec3f(&atOffset, &cam->eye);
	
	Matrix_Pop();
}

void View_Camera_OrbitMode(View3D* this, Input* inputCtx) {
	Camera* cam = this->currentCamera;
	f32 distMult = (cam->dist * 0.2);
	
	if (this->cameraControl) {
		if (inputCtx->key[KEY_LEFT_CONTROL].hold && inputCtx->mouse.clickMid.hold && inputCtx->mouse.scrollY == 0)
			cam->targetDist += inputCtx->mouse.vel.y;
		
		f32 disdiff = fabsf(cam->dist - cam->targetDist);
		f32 fovDiff = fabsf(this->fovy - this->fovyTarget);
		
		if (inputCtx->mouse.clickMid.hold || inputCtx->mouse.scrollY || disdiff || fovDiff) {
			if (inputCtx->key[KEY_LEFT_CONTROL].hold && inputCtx->mouse.scrollY) {
				cam->targetDist = cam->dist;
				this->fovyTarget = Clamp(this->fovyTarget * (1.0 + (inputCtx->mouse.scrollY / 20)), 30, 120);
				fovDiff = -this->fovy;
			} else {
				if (inputCtx->mouse.scrollY) {
					cam->targetDist -= inputCtx->mouse.scrollY * distMult * 0.75f;
				}
				
				cam->targetDist = ClampMin(cam->targetDist, 1.0f);
				Math_DelSmoothStepToF(&cam->dist, cam->targetDist, 0.25f, 5.0f * distMult, 0.01f * distMult);
			}
			
			if (fovDiff) {
				f32 f = -this->fovy;
				Math_DelSmoothStepToF(&this->fovy, this->fovyTarget, 0.25, 5.25f, 0.0f);
				f += this->fovy;
				
				cam->offset.z += f * 8.f * (1.0f - this->fovy / 150);
			}
			
			if (inputCtx->mouse.clickMid.hold) {
				if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
					cam->offset.y += inputCtx->mouse.vel.y * distMult * 0.01f;
					cam->offset.x += inputCtx->mouse.vel.x * distMult * 0.01f;
				} else if (inputCtx->key[KEY_LEFT_CONTROL].hold == false) {
					cam->yaw -= inputCtx->mouse.vel.x * 67;
					cam->pitch += inputCtx->mouse.vel.y * 67;
				}
			}
			
			Camera_CalculateOrbit(cam);
		}
	}
}

void View_Init(View3D* this, Input* inputCtx) {
	Camera* cam;
	
	this->currentCamera = &this->camera[0];
	cam = this->currentCamera;
	
	cam->dist = cam->targetDist = 300.f;
	cam->speedMod = 5.0f;
	
	Camera_CalculateFly(cam);
	Matrix_LookAt(&this->viewMtx, cam->eye, cam->at, cam->up);
	
	this->fovyTarget = this->fovy = 65;
	this->near = 10.0;
	this->far = 12800.0;
	this->scale = 1;
	this->mode = CAM_MODE_ALL;
}

void View_Update(View3D* this, Input* inputCtx) {
	Camera* cam = this->currentCamera;
	
	void (*camMode[])(View3D*, Input*) = {
		View_Camera_FlyMode,
		View_Camera_OrbitMode,
	};
	
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
	
	if (this->mode == CAM_MODE_ALL)
		for (s32 i = 0; i < ArrayCount(camMode); i++)
			camMode[i](this, inputCtx);
	
	else
		camMode[this->mode](this, inputCtx);
	
	if (this->moveToTarget && inputCtx->mouse.clickL.hold == false) {
		Vec3f p = this->targetPos;
		
		f32 x = Math_DelSmoothStepToF(&this->targetPos.x, 0, 0.25f, this->targetStep, 0.001f);
		f32 z = Math_DelSmoothStepToF(&this->targetPos.z, 0, 0.25f, this->targetStep, 0.001f);
		f32 y = Math_DelSmoothStepToF(&this->targetPos.y, 0, 0.25f, this->targetStep, 0.001f);
		
		p = Math_Vec3f_Sub(p, this->targetPos);
		
		Camera_CalculateMove(cam, p.x, p.y, p.z);
		
		if (x + z + y < 0.1f)
			this->moveToTarget = false;
	} else
		this->moveToTarget = false;
	
	Matrix_LookAt(&this->viewMtx, cam->eye, cam->at, cam->up);
	
	Matrix_Scale(1.0, 1.0, 1.0, MTXMODE_NEW);
	Matrix_ToMtxF(&this->modelMtx);
}

void View_SetProjectionDimensions(View3D* this, Vec2s* dim) {
	this->projectDim = *dim;
}

RayLine View_Raycast(View3D* this, Vec2s pos, Rect dispRect) {
	s32 viewport[] = { 0, 0, dispRect.w, dispRect.h };
	MtxF modelview;
	Vec2s mouse = Math_Vec2s_New(pos.x, dispRect.h - pos.y);
	Vec3f a, b;
	
	Matrix_MtxFMtxFMult(&this->modelMtx, &this->viewMtx, &modelview);
	glhUnProjectf(mouse.x, mouse.y, 0, (float*)&modelview, (float*)&this->projMtx, viewport, (float*)&a);
	glhUnProjectf(mouse.x, mouse.y, 1, (float*)&modelview, (float*)&this->projMtx, viewport, (float*)&b);
	
	return RayLine_New(a, b);
}

void View_MoveTo(View3D* this, Vec3f pos) {
	this->targetPos = Math_Vec3f_Sub(pos, this->currentCamera->at);
	this->moveToTarget = true;
	this->targetStep = Math_Vec3f_DistXYZ(pos, this->currentCamera->at) * 0.25f;
}

// # # # # # # # # # # # # # # # # # # # #
// # gluUnProject                        #
// # # # # # # # # # # # # # # # # # # # #
// https://www.khronos.org/opengl/wiki/GluProject_and_gluUnProject_code

int glhProjectf(float objx, float objy, float objz, float* modelview, float* projection, int* viewport, float* windowCoordinate) {
	// Transformation vectors
	float fTempo[8];
	
	// Modelview transform
	fTempo[0] = modelview[0] * objx + modelview[4] * objy + modelview[8] * objz + modelview[12]; // w is always 1
	fTempo[1] = modelview[1] * objx + modelview[5] * objy + modelview[9] * objz + modelview[13];
	fTempo[2] = modelview[2] * objx + modelview[6] * objy + modelview[10] * objz + modelview[14];
	fTempo[3] = modelview[3] * objx + modelview[7] * objy + modelview[11] * objz + modelview[15];
	// Projection transform, the final row of projection matrix is always [0 0 -1 0]
	// so we optimize for that.
	fTempo[4] = projection[0] * fTempo[0] + projection[4] * fTempo[1] + projection[8] * fTempo[2] + projection[12] * fTempo[3];
	fTempo[5] = projection[1] * fTempo[0] + projection[5] * fTempo[1] + projection[9] * fTempo[2] + projection[13] * fTempo[3];
	fTempo[6] = projection[2] * fTempo[0] + projection[6] * fTempo[1] + projection[10] * fTempo[2] + projection[14] * fTempo[3];
	fTempo[7] = -fTempo[2];
	// The result normalizes between -1 and 1
	if (fTempo[7]==0.0)    // The w value
		return 0;
	fTempo[7] = 1.0 / fTempo[7];
	// Perspective division
	fTempo[4] *= fTempo[7];
	fTempo[5] *= fTempo[7];
	fTempo[6] *= fTempo[7];
	// Window coordinates
	// Map x, y to range 0-1
	windowCoordinate[0] = (fTempo[4] * 0.5 + 0.5) * viewport[2] + viewport[0];
	windowCoordinate[1] = (fTempo[5] * 0.5 + 0.5) * viewport[3] + viewport[1];
	// This is only correct when glDepthRange(0.0, 1.0)
	windowCoordinate[2] = (1.0 + fTempo[6]) * 0.5; // Between 0 and 1
	
	return 1;
}

static void MultiplyMatrices4by4OpenGL_FLOAT(float* result, float* matrix1, float* matrix2) {
	result[0] = matrix1[0] * matrix2[0] +
		matrix1[4] * matrix2[1] +
		matrix1[8] * matrix2[2] +
		matrix1[12] * matrix2[3];
	result[4] = matrix1[0] * matrix2[4] +
		matrix1[4] * matrix2[5] +
		matrix1[8] * matrix2[6] +
		matrix1[12] * matrix2[7];
	result[8] = matrix1[0] * matrix2[8] +
		matrix1[4] * matrix2[9] +
		matrix1[8] * matrix2[10] +
		matrix1[12] * matrix2[11];
	result[12] = matrix1[0] * matrix2[12] +
		matrix1[4] * matrix2[13] +
		matrix1[8] * matrix2[14] +
		matrix1[12] * matrix2[15];
	result[1] = matrix1[1] * matrix2[0] +
		matrix1[5] * matrix2[1] +
		matrix1[9] * matrix2[2] +
		matrix1[13] * matrix2[3];
	result[5] = matrix1[1] * matrix2[4] +
		matrix1[5] * matrix2[5] +
		matrix1[9] * matrix2[6] +
		matrix1[13] * matrix2[7];
	result[9] = matrix1[1] * matrix2[8] +
		matrix1[5] * matrix2[9] +
		matrix1[9] * matrix2[10] +
		matrix1[13] * matrix2[11];
	result[13] = matrix1[1] * matrix2[12] +
		matrix1[5] * matrix2[13] +
		matrix1[9] * matrix2[14] +
		matrix1[13] * matrix2[15];
	result[2] = matrix1[2] * matrix2[0] +
		matrix1[6] * matrix2[1] +
		matrix1[10] * matrix2[2] +
		matrix1[14] * matrix2[3];
	result[6] = matrix1[2] * matrix2[4] +
		matrix1[6] * matrix2[5] +
		matrix1[10] * matrix2[6] +
		matrix1[14] * matrix2[7];
	result[10] = matrix1[2] * matrix2[8] +
		matrix1[6] * matrix2[9] +
		matrix1[10] * matrix2[10] +
		matrix1[14] * matrix2[11];
	result[14] = matrix1[2] * matrix2[12] +
		matrix1[6] * matrix2[13] +
		matrix1[10] * matrix2[14] +
		matrix1[14] * matrix2[15];
	result[3] = matrix1[3] * matrix2[0] +
		matrix1[7] * matrix2[1] +
		matrix1[11] * matrix2[2] +
		matrix1[15] * matrix2[3];
	result[7] = matrix1[3] * matrix2[4] +
		matrix1[7] * matrix2[5] +
		matrix1[11] * matrix2[6] +
		matrix1[15] * matrix2[7];
	result[11] = matrix1[3] * matrix2[8] +
		matrix1[7] * matrix2[9] +
		matrix1[11] * matrix2[10] +
		matrix1[15] * matrix2[11];
	result[15] = matrix1[3] * matrix2[12] +
		matrix1[7] * matrix2[13] +
		matrix1[11] * matrix2[14] +
		matrix1[15] * matrix2[15];
}

static void MultiplyMatrixByVector4by4OpenGL_FLOAT(float* resultvector, const float* matrix, const float* pvector) {
	resultvector[0] = matrix[0] * pvector[0] + matrix[4] * pvector[1] + matrix[8] * pvector[2] + matrix[12] * pvector[3];
	resultvector[1] = matrix[1] * pvector[0] + matrix[5] * pvector[1] + matrix[9] * pvector[2] + matrix[13] * pvector[3];
	resultvector[2] = matrix[2] * pvector[0] + matrix[6] * pvector[1] + matrix[10] * pvector[2] + matrix[14] * pvector[3];
	resultvector[3] = matrix[3] * pvector[0] + matrix[7] * pvector[1] + matrix[11] * pvector[2] + matrix[15] * pvector[3];
}

#define SWAP_ROWS_DOUBLE(a, b) do { double* _tmp = a; (a) = (b); (b) = _tmp; } while (0)
#define SWAP_ROWS_FLOAT(a, b)  do { float* _tmp = a; (a) = (b); (b) = _tmp; } while (0)
#define MAT(m, r, c)           (m)[(c) * 4 + (r)]

// This code comes directly from GLU except that it is for float
static int glhInvertMatrixf2(float* m, float* out) {
	float wtmp[4][8];
	float m0, m1, m2, m3, s;
	float* r0, * r1, * r2, * r3;
	
	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
	r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
	r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
	r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
	r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
	r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
	r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
	r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
	r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
	r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
	r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
	r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
	/* choose pivot - or die */
	if (fabsf(r3[0]) > fabsf(r2[0]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (fabsf(r2[0]) > fabsf(r1[0]))
		SWAP_ROWS_FLOAT(r2, r1);
	if (fabsf(r1[0]) > fabsf(r0[0]))
		SWAP_ROWS_FLOAT(r1, r0);
	if (0.0 == r0[0])
		return 0;
	/* eliminate first variable */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}
	/* choose pivot - or die */
	if (fabsf(r3[1]) > fabsf(r2[1]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (fabsf(r2[1]) > fabsf(r1[1]))
		SWAP_ROWS_FLOAT(r2, r1);
	if (0.0 == r1[1])
		return 0;
	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}
	/* choose pivot - or die */
	if (fabsf(r3[2]) > fabsf(r2[2]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (0.0 == r2[2])
		return 0;
	/* eliminate third variable */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
	r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
	/* last check */
	if (0.0 == r3[3])
		return 0;
	s = 1.0 / r3[3];       /* now back substitute row 3 */
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;
	m2 = r2[3];            /* now back substitute row 2 */
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
	r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
	r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
	r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
	m1 = r1[2];            /* now back substitute row 1 */
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
	r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
	m0 = r0[1];            /* now back substitute row 0 */
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];
	
	return 1;
}

static int glhUnProjectf(float winx, float winy, float winz, float* modelview, float* projection, int* viewport, float* objectCoordinate) {
	// Transformation matrices
	float m[16], A[16];
	float in[4], out[4];
	
	// Calculation for inverting a matrix, compute projection x modelview
	// and store in A[16]
	MultiplyMatrices4by4OpenGL_FLOAT(A, projection, modelview);
	// Now compute the inverse of matrix A
	if (glhInvertMatrixf2(A, m)==0)
		return 0;
	// Transformation of normalized coordinates between -1 and 1
	in[0] = (winx - (float)viewport[0]) / (float)viewport[2] * 2.0 - 1.0;
	in[1] = (winy - (float)viewport[1]) / (float)viewport[3] * 2.0 - 1.0;
	in[2] = 2.0 * winz - 1.0;
	in[3] = 1.0;
	// Objects coordinates
	MultiplyMatrixByVector4by4OpenGL_FLOAT(out, m, in);
	if (out[3]==0.0)
		return 0;
	out[3] = 1.0 / out[3];
	objectCoordinate[0] = out[0] * out[3];
	objectCoordinate[1] = out[1] * out[3];
	objectCoordinate[2] = out[2] * out[3];
	
	return 1;
}
