#include "ext_interface.h"

/*============================================================================*/

static void Camera_CalculateFly(Camera* cam) {
    Vec3f zero = Math_Vec3f_New(0, 0, 0);
    Vec3f upOffset = Math_Vec3f_New(0, 1, 0);
    Vec3f rOffset = Math_Vec3f_New(1, 0, 0);
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
    Matrix_MultVec3f(&rOffset, &cam->right);
    cam->up = Math_Vec3f_Sub(cam->up, cam->eye);
    cam->right = Math_Vec3f_Sub(cam->right, cam->eye);
    Matrix_MultVec3f(&atOffset, &cam->at);
    
    Matrix_Pop();
}

static void Camera_FlyMode(View3D* this, Input* inputCtx, bool update) {
    Camera* cam = this->currentCamera;
    
    if (this->cameraControl && update) {
        f32 step = 1.0f * cam->speedMod;
        f32 min = 0.01f * cam->speedMod;
        
        if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
            step *= 4;
            Math_DelSmoothStepToF(&cam->speed, cam->speedMod * 4, 0.25f, 1.00f, 0.1f);
        } else if (inputCtx->key[KEY_SPACE].hold) {
            step *= 8;
            Math_DelSmoothStepToF(&cam->speed, cam->speedMod * 8, 0.25f, 1.00f, 0.1f);
        } else {
            Math_DelSmoothStepToF(&cam->speed, cam->speedMod, 0.25f, 1.00f, 0.1f);
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
        
        if (Input_GetMouse(inputCtx, CLICK_L)->hold) {
            const Vec3f up = { 0, 1, 0 };
            f32 dot = Math_Vec3f_Dot(cam->up, up);
            s32 s = dot >= 0.0f ? 1 : -1;
            
            dot = powf(fabsf(dot), 0.5f) * s;
            
            cam->pitch = (s32)(cam->pitch + inputCtx->cursor.vel.y * 50.5f);
            cam->yaw = (s32)(cam->yaw - inputCtx->cursor.vel.x * 50.5f * dot);
        }
    } else {
        Math_DelSmoothStepToF(&cam->speed, 0.5f, 0.5f, 1.00f, 0.1f);
        Math_DelSmoothStepToF(&cam->vel.x, 0.0f, 0.5f, 1.00f, 0.1f);
        Math_DelSmoothStepToF(&cam->vel.z, 0.0f, 0.5f, 1.00f, 0.1f);
        Math_DelSmoothStepToF(&cam->vel.y, 0.0f, 0.5f, 1.00f, 0.1f);
    }
    
    Camera_CalculateFly(cam);
}

/*============================================================================*/

static void Camera_CalculateOrbit(Camera* cam) {
    Vec3f zero = Math_Vec3f_New(0, 0, 0);
    Vec3f upOffset = Math_Vec3f_New(0, 1, 0);
    Vec3f rOffset = Math_Vec3f_New(1, 0, 0);
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
    Matrix_MultVec3f(&rOffset, &cam->right);
    cam->up = Math_Vec3f_Sub(cam->up, cam->at);
    cam->right = Math_Vec3f_Sub(cam->right, cam->at);
    Matrix_MultVec3f(&atOffset, &cam->eye);
    
    Matrix_Pop();
}

static void Camera_OrbitMode(View3D* this, Input* inputCtx, bool update) {
    Camera* cam = this->currentCamera;
    f32 distMult = (cam->dist * 0.2);
    f32 disdiff = fabsf(cam->dist - cam->targetDist);
    f32 fovDiff = fabsf(this->fovy - this->fovyTarget);
    
    if (this->cameraControl && update) {
        if (inputCtx->key[KEY_LEFT_CONTROL].hold && inputCtx->cursor.clickMid.hold && Input_GetScroll(inputCtx) == 0)
            cam->targetDist += inputCtx->cursor.vel.y;
        
        if (inputCtx->cursor.clickMid.hold || Input_GetScroll(inputCtx) || disdiff || fovDiff) {
            if (inputCtx->key[KEY_LEFT_CONTROL].hold && Input_GetScroll(inputCtx)) {
                cam->targetDist = cam->dist;
                this->fovyTarget = clamp(this->fovyTarget * (1.0 + (Input_GetScroll(inputCtx) / 20)), 10, 130);
                fovDiff = -this->fovy;
            } else {
                if (Input_GetScroll(inputCtx)) {
                    cam->targetDist -= Input_GetScroll(inputCtx) * distMult * 0.75f;
                }
                
                cam->targetDist = clamp_min(cam->targetDist, 1.0f);
            }
            
            if (inputCtx->cursor.clickMid.hold) {
                if (inputCtx->key[KEY_LEFT_SHIFT].hold) {
                    cam->offset.y += inputCtx->cursor.vel.y * distMult * 0.01f;
                    cam->offset.x += inputCtx->cursor.vel.x * distMult * 0.01f;
                } else if (inputCtx->key[KEY_LEFT_CONTROL].hold == false) {
                    cam->yaw -= inputCtx->cursor.vel.x * 67;
                    cam->pitch += inputCtx->cursor.vel.y * 67;
                }
            }
        }
    }
    
    Math_DelSmoothStepToF(&cam->dist, cam->targetDist, 0.25f, 5.0f * distMult, 0.01f * distMult);
    if (fovDiff) {
        f32 f = -this->fovy;
        Math_DelSmoothStepToF(&this->fovy, this->fovyTarget, 0.25, 5.25f, 0.0f);
        f += this->fovy;
        
        cam->offset.z += f * 8.f * (1.0f - this->fovy / 150);
    }
    
    Camera_CalculateOrbit(cam);
}

/*============================================================================*/

static void Camera_CalculateMove(Camera* cam, f32 x, f32 y, f32 z) {
    Vec3f zero = Math_Vec3f_New(0, 0, 0);
    Vec3f upOffset = Math_Vec3f_New(0, 1, 0);
    Vec3f rOffset = Math_Vec3f_New(1, 0, 0);
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
    Matrix_MultVec3f(&rOffset, &cam->right);
    
    cam->up = Math_Vec3f_Sub(cam->up, cam->at);
    cam->right = Math_Vec3f_Sub(cam->right, cam->at);
    Matrix_MultVec3f(&atOffset, &cam->eye);
    
    Matrix_Pop();
}

static void Camera_Update_MoveTo(View3D* this, Input* input) {
    Camera* cam = this->currentCamera;
    
    if (this->moveToTarget) {
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
}

static void Camera_Update_RotTo(View3D* this, Input* input) {
    Camera* cam = this->currentCamera;
    u8 isSwapping = this->rotToAngle;
    
    if (this->cameraControl && !this->interrupt) {
        s32 pass = -1;
        s32 key[] = {
            KEY_KP_1,
            KEY_KP_3,
            KEY_KP_7,
        };
        Vec3f target[] = {
            { 0,  0,   0   },
            { 0,  90,  0   },
            { 90, 0,   0   },
        };
        
        for (int i = 0; i < ArrCount(key); i++) {
            if (Input_GetKey(input, key[i])->press) {
                pass = i;
                break;
            }
        }
        
        if (pass > -1) {
            this->rotToAngle = true;
            
            for (int i = 0; i < 3; i++)
                this->targetRot.axis[i] = DegToBin(target[pass].axis[i]);
        } else {
            if (Input_GetKey(input, KEY_KP_9)->press) {
                // Invert
                Vec3f front;
                Vec3f up;
                f32 fdot;
                
                this->rotToAngle = true;
                if (isSwapping)
                    front = Math_Vec3f_New(SinS(this->targetRot.y), SinS(-this->targetRot.x), CosS(this->targetRot.y));
                
                else
                    front = Math_Vec3f_LineSegDir(cam->eye, cam->at);
                
                up = Math_Vec3f_New(0, 1, 0);
                fdot = fabsf(Math_Vec3f_Dot(front, up));
                
                if (fdot < 0.9995f) {
                    this->targetRot.y += DegToBin(180);
                } else {
                    this->targetRot.x += DegToBin(180);
                }
            } else {
                Vec3s r = Math_Vec3s_New(cam->pitch, cam->yaw, cam->roll);
                Vec3s p =  { 0, 0, 0 };
                
                if (Input_GetKey(input, KEY_KP_4)->press || Input_GetKey(input, KEY_KP_6)->press) {
                    s32 m = Input_GetKey(input, KEY_KP_4)->press ? -1 : 1;
                    
                    // Sideways
                    this->rotToAngle = true;
                    p.y += DegToBin(15) * m;
                }
                
                if (Input_GetKey(input, KEY_KP_2)->press || Input_GetKey(input, KEY_KP_8)->press) {
                    s32 m = Input_GetKey(input, KEY_KP_2)->press ? -1 : 1;
                    
                    // Sideways
                    this->rotToAngle = true;
                    p.x += DegToBin(15) * m;
                }
                
                if (isSwapping)
                    p = Math_Vec3s_Add(p, Math_Vec3s_Sub(this->targetRot, r));
                
                this->targetRot = Math_Vec3s_Add(r, p);
            }
        }
        
        if (Input_GetKey(input, KEY_KP_5)->press)
            this->ortho ^= true;
    }
    
    if (this->rotToAngle) {
        s16 x = Math_SmoothStepToS(&cam->pitch, this->targetRot.x, 3, DegToBin(45), 1);
        s16 z = Math_SmoothStepToS(&cam->yaw, this->targetRot.y, 3, DegToBin(45), 1);
        s16 y = Math_SmoothStepToS(&cam->roll, this->targetRot.z, 3, DegToBin(45), 1);
        
        Camera_CalculateOrbit(cam);
        
        if (!x && !y && !z)
            this->rotToAngle = false;
    } else
        this->rotToAngle = false;
}

/*============================================================================*/

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

void View_InterruptControl(View3D* this) {
    this->interrupt = true;
}

void View_Update(View3D* this, Input* inputCtx, Split* split) {
    void (*camMode[])(View3D*, Input*, bool) = {
        Camera_FlyMode,
        Camera_OrbitMode,
    };
    Camera* cam = this->currentCamera;
    
    this->split = split;
    this->usePreCalcRay = false;
    this->isControlled = false;
    
    if (this->ortho)
        Matrix_Ortho(
            &this->projMtx, cam->dist, (f32)this->split->rect.w / (f32)this->split->rect.h, this->near, this->far);
    else
        Matrix_Projection(
            &this->projMtx, this->fovy, (f32)this->split->rect.w / (f32)this->split->rect.h, this->near, this->far, this->scale);
    
    for (int i = 0; i < ArrCount(camMode); i++)
        camMode[i](this, inputCtx, (this->mode & (1 << i) && !this->interrupt));
    
    if (this->cameraControl && !this->interrupt) {
        s32 keyList[] = { KEY_W, KEY_A, KEY_S, KEY_D };
        
        for (int i = 0; i < ArrCount(keyList); i++) {
            if (Input_GetKey(inputCtx, keyList[i])->hold)
                this->isControlled = true;
        }
    }
    
    Camera_Update_MoveTo(this, inputCtx);
    Camera_Update_RotTo(this, inputCtx);
    
    Matrix_LookAt(&this->viewMtx, cam->eye, cam->at, cam->up);
    
    Matrix_Scale(1.0, 1.0, 1.0, MTXMODE_NEW);
    Matrix_ToMtxF(&this->modelMtx);
    
    Matrix_MtxFMtxFMult(&this->projMtx, &this->viewMtx, &this->projViewMtx);
    this->interrupt = false;
}

/*============================================================================*/

bool View_CheckControlKeys(Input* input) {
    if (Input_GetMouse(input, CLICK_L)->hold)
        return true;
    if (Input_GetMouse(input, CLICK_M)->hold)
        return true;
    if (Input_GetKey(input, KEY_W)->hold)
        return true;
    if (Input_GetKey(input, KEY_A)->hold)
        return true;
    if (Input_GetKey(input, KEY_S)->hold)
        return true;
    if (Input_GetKey(input, KEY_D)->hold)
        return true;
    if (Input_GetKey(input, KEY_Q)->hold)
        return true;
    if (Input_GetKey(input, KEY_E)->hold)
        return true;
    for (int i = KEY_KP_0; i <= KEY_KP_9; i++)
        if (Input_GetKey(input, i)->hold)
            return true;
    if (Input_GetScroll(input))
        return true;
    
    return false;
}

RayLine View_GetPointRayLine(View3D* this, Vec2f point) {
    Rect dispRect = this->split->dispRect;
    Vec3f projA = Math_Vec3f_New(point.x, point.y, 0.0f);
    Vec3f projB = Math_Vec3f_New(point.x, point.y, 1.0f);
    Vec3f rayA, rayB;
    
    Matrix_Unproject(&this->viewMtx, &this->projMtx, &projA, &rayA, dispRect.w, dispRect.h);
    Matrix_Unproject(&this->viewMtx, &this->projMtx, &projB, &rayB, dispRect.w, dispRect.h);
    
    return RayLine_New(rayA, rayB);
}

RayLine View_GetCursorRayLine(View3D* this) {
    if (this->usePreCalcRay)
        return this->ray;
    
    this->usePreCalcRay = true;
    
    return this->ray = View_GetPointRayLine(this, Math_Vec2f_New(UnfoldVec2(this->split->cursorPos)));
}

MtxF View_GetOrientedMtxF(View3D* this, f32 x, f32 y, f32 z) {
    Camera* curCam = this->currentCamera;
    MtxF orient;
    
    Matrix_Push();
    Matrix_RotateY_s(curCam->yaw, MTXMODE_NEW);
    Matrix_RotateX_s(curCam->pitch, MTXMODE_APPLY);
    
    Matrix_RotateY_d(x, MTXMODE_APPLY);
    Matrix_RotateX_d(y, MTXMODE_APPLY);
    Matrix_RotateZ_d(z, MTXMODE_APPLY);
    
    Matrix_RotateX_s(-curCam->pitch, MTXMODE_APPLY);
    Matrix_RotateY_s(-curCam->yaw, MTXMODE_APPLY);
    Matrix_Get(&orient);
    Matrix_Pop();
    
    return orient;
}

MtxF View_GetLockOrientedMtxF(View3D* this, f32 dgr, int axis_id, bool viewFix) {
    Vec3f proj[3] = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
    };
    Vec3f camDir = Math_Vec3f_LineSegDir(this->currentCamera->eye, this->currentCamera->at);
    
    camDir = Math_Vec3f_Project(camDir, proj[axis_id]);
    camDir = Math_Vec3f_Normalize(camDir);
    dgr = DegToRad(dgr);
    
    if (viewFix)
        if (camDir.axis[axis_id] >= 0.0f)
            dgr = -dgr;
    
    MtxF axis[] = {
        {
            1.0f,       0.0f,            0.0f,                 0.0f,
            0.0f,       cosf(dgr),       -sinf(dgr),           0.0f,
            0.0f,       sinf(dgr),       cosf(dgr),            0.0f,
            0.0f,       0.0f,            0.0f,                 1.0f,
        },
        {
            cosf(dgr),  0.0f,            sinf(dgr),            0.0f,
            0.0f,       1.0f,            0.0f,                 0.0f,
            -sinf(dgr), 0.0f,            cosf(dgr),            0.0f,
            0.0f,       0.0f,            0.0f,                 1.0f,
        },
        {
            cosf(dgr),  -sinf(dgr),      0.0f,                 0.0f,
            sinf(dgr),  cosf(dgr),       0.0f,                 0.0f,
            0.0f,       0.0f,            1.0f,                 0.0f,
            0.0f,       0.0f,            0.0f,                 1.0f,
        },
    };
    
    return axis[axis_id];
}

void View_MoveTo(View3D* this, Vec3f pos) {
    this->targetPos = Math_Vec3f_Sub(pos, this->currentCamera->at);
    this->moveToTarget = true;
    this->targetStep = Math_Vec3f_DistXYZ(pos, this->currentCamera->at) * 0.25f;
}

void View_ZoomTo(View3D* this, f32 zoom) {
    this->currentCamera->targetDist = zoom;
}

void View_RotTo(View3D* this, Vec3s rot) {
    this->targetRot.x = rot.x;
    this->targetRot.y = rot.y;
    this->targetRot.z = rot.z;
    this->rotToAngle = true;
}

Vec3f View_OrientDirToView(View3D* this, Vec3f dir) {
    Vec3f pos;
    
    Matrix_Push();
    Matrix_RotateY_s(this->currentCamera->yaw, MTXMODE_APPLY);
    Matrix_RotateX_s(this->currentCamera->pitch, MTXMODE_APPLY);
    Matrix_RotateZ_s(this->currentCamera->roll, MTXMODE_APPLY);
    Matrix_MultVec3f(&dir, &pos);
    Matrix_Pop();
    
    return pos;
}

Vec2f View_GetScreenPos(View3D* this, Vec3f point) {
    f32 w = this->split->dispRect.w * 0.5f;
    f32 h = this->split->dispRect.h * 0.5f;
    Vec4f pos;
    
    Matrix_MultVec3fToVec4f_Ext(&point, &pos, &this->projViewMtx);
    
    return Math_Vec2f_New(
        w + (pos.x / pos.w) * w,
        h - (pos.y / pos.w) * h
    );
}

bool View_PointInScreen(View3D* this, Vec3f point) {
    Vec4f pos;
    
    // Vec2f p2;
    // Rect r = {
    //  0, 0, this->split->dispRect.w, this->split->dispRect.h
    // };
    
    if (this->ortho)
        return true;
    
    Matrix_MultVec3fToVec4f_Ext(&point, &pos, &this->projViewMtx);
    
    if (pos.z <= 0)
        return false;
    
    // p2 = View_GetScreenPos(this, point);
    //
    // if (!Rect_PointIntersect(&r, p2.x, p2.y))
    //  return false;
    
    return true;
}

void View_ClipPointIntoView(View3D* this, Vec3f* a, Vec3f normal) {
    Vec4f test;
    f32 dot;
    
    Matrix_MultVec3fToVec4f_Ext(a, &test, &this->projViewMtx);
    
    if (test.w <= 0.0f) {
        dot = Math_Vec3f_Dot(normal, Math_Vec3f_LineSegDir(this->currentCamera->eye, this->currentCamera->at));
        *a = Math_Vec3f_Add(*a, Math_Vec3f_MulVal(normal, -test.w / dot + 1.0f));
    }
}

f32 View_DepthFactor(View3D* this, Vec3f point) {
    Vec4f sp;
    
    Matrix_MultVec3fToVec4f_Ext(&point, &sp, &this->projViewMtx);
    
    return sp.w;
}

f32 View_DimFactor(View3D* this) {
    return sqrtf(this->split->rect.w * this->split->rect.h);
}

/*============================================================================*/