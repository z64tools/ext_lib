#ifndef EXT_VECTOR_H
#define EXT_VECTOR_H

#include "ext_type.h"
#include "ext_math.h"
#include "ext_macros.h"
#include <math.h>

#define veccmp(a, b) ({                                             \
        bool r = false;                                             \
        for (int i = 0; i < sizeof((*(a))) / sizeof((*(a)).x); i++) \
        if ((*(a)).axis[i] != (*(b)).axis[i]) r = true;             \
        r;                                                          \
    })

#define veccpy(a, b) do {                                       \
    for (int i = 0; i < sizeof((*(a))) / sizeof((*(a)).x); i++) \
    (*(a)).axis[i] = (*(b)).axis[i];                            \
} while (0)

#define SQ(x)   ((x) * (x))
#define SinS(x) sinf(BinToRad((s16)(x)))
#define CosS(x) cosf(BinToRad((s16)(x)))
#define SinF(x) sinf(DegToRad(x))
#define CosF(x) cosf(DegToRad(x))
#define SinR(x) sinf(x)
#define CosR(x) cosf(x)

// Trig macros
#define DegToBin(degreesf) (s16)(s32)(degreesf * 182.04167f + .5f)
#define RadToBin(radf)     (s16)(s32)(radf * (32768.0f / M_PI))
#define RadToDeg(radf)     (radf * (180.0f / M_PI))
#define DegToRad(degf)     (degf * (M_PI / 180.0f))
#define BinFlip(angle)     ((s16)(angle - 0x7FFF))
#define BinSub(a, b)       ((s16)(a - b))
#define BinToDeg(binang)   ((f32)(s32)binang * (360.0001525f / 65535.0f))
#define BinToRad(binang)   (((f32)(s32)binang / 32768.0f) * M_PI)

#define UnfoldRect(rect) (rect).x, (rect).y, (rect).w, (rect).h
#define UnfoldVec2(vec)  (vec).x, (vec).y
#define UnfoldVec3(vec)  (vec).x, (vec).y, (vec).z
#define UnfoldVec4(vec)  (vec).x, (vec).y, (vec).z, (vec).w
#define IsZero(f32)      ((fabsf(f32) < EPSILON))

#define VEC_TYPE(type, suffix) \
    typedef union {            \
        struct {               \
            type x;            \
            type y;            \
            type z;            \
            type w;            \
        };                     \
        type axis[4];          \
    } Vec4 ## suffix;          \
    typedef union {            \
        struct {               \
            type x;            \
            type y;            \
            type z;            \
        };                     \
        type axis[3];          \
    } Vec3 ## suffix;          \
    typedef union {            \
        struct {               \
            type x;            \
            type y;            \
        };                     \
        type axis[2];          \
    } Vec2 ## suffix;

VEC_TYPE(f32, f)
VEC_TYPE(s16, s)

typedef struct {
    f32 r;
    s16 pitch;
    s16 yaw;
} VecSph;

typedef struct {
    f32 x;
    f32 y;
    f32 w;
    f32 h;
} Rectf32;

typedef struct {
    s32 x;
    s32 y;
    s32 w;
    s32 h;
} Rect;

typedef struct {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;
} CRect;

typedef struct {
    f32 xMin, xMax;
    f32 zMin, zMax;
    f32 yMin, yMax;
} BoundBox;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// # # # # # # # # # # # # # # # # # # # #
// # Forward Declaration                 #
// # # # # # # # # # # # # # # # # # # # #

static inline f32 Math_Vec2s_DistXZ(Vec2s a, Vec2s b);

extern const f32 EPSILON;
extern f32 gDeltaTime;

// # # # # # # # # # # # # # # # # # # # #
// # F U N C T I O N S                   #
// # # # # # # # # # # # # # # # # # # # #

static inline s16 Atan2S(f32 x, f32 y) {
    return RadToBin(atan2f(y, x));
}

// # # # # # # # # # # # # # # # # # # # #
// # VecSph                              #
// # # # # # # # # # # # # # # # # # # # #

static inline void Math_VecSphToVec3f(Vec3f* dest, VecSph* sph) {
    sph->pitch = clamp(sph->pitch, DegToBin(-89.995f), DegToBin(89.995f));
    dest->x = sph->r * SinS(sph->pitch - DegToBin(90)) * SinS(sph->yaw);
    dest->y = sph->r * CosS(sph->pitch - DegToBin(90));
    dest->z = sph->r * SinS(sph->pitch - DegToBin(90)) * CosS(sph->yaw);
}

static inline void Math_AddVecSphToVec3f(Vec3f* dest, VecSph* sph) {
    sph->pitch = clamp(sph->pitch, DegToBin(-89.995f), DegToBin(89.995f));
    dest->x += sph->r * SinS(sph->pitch - DegToBin(90)) * SinS(sph->yaw);
    dest->y += sph->r * CosS(sph->pitch - DegToBin(90));
    dest->z += sph->r * SinS(sph->pitch - DegToBin(90)) * CosS(sph->yaw);
}

static inline VecSph* Math_Vec3fToVecSph(VecSph* dest, Vec3f* vec) {
    VecSph sph;
    
    f32 distSquared = SQ(vec->x) + SQ(vec->z);
    f32 dist = sqrtf(distSquared);
    
    if ((dist == 0.0f) && (vec->y == 0.0f)) {
        sph.pitch = 0;
    } else {
        sph.pitch = DegToBin(RadToDeg(atan2(dist, vec->y)));
    }
    
    sph.r = sqrtf(SQ(vec->y) + distSquared);
    if ((vec->x == 0.0f) && (vec->z == 0.0f)) {
        sph.yaw = 0;
    } else {
        sph.yaw = DegToBin(RadToDeg(atan2(vec->x, vec->z)));
    }
    
    *dest = sph;
    
    return dest;
}

static inline VecSph* Math_Vec3fToVecSphGeo(VecSph* dest, Vec3f* vec) {
    VecSph sph;
    
    Math_Vec3fToVecSph(&sph, vec);
    sph.pitch = 0x3FFF - sph.pitch;
    
    *dest = sph;
    
    return dest;
}

static inline VecSph* Math_Vec3fDiffToVecSphGeo(VecSph* dest, Vec3f* a, Vec3f* b) {
    Vec3f sph;
    
    sph.x = b->x - a->x;
    sph.y = b->y - a->y;
    sph.z = b->z - a->z;
    
    return Math_Vec3fToVecSphGeo(dest, &sph);
}

static inline Vec3f* Math_CalcUpFromPitchYawRoll(Vec3f* dest, s16 pitch, s16 yaw, s16 roll) {
    f32 sinPitch;
    f32 cosPitch;
    f32 sinYaw;
    f32 cosYaw;
    f32 sinNegRoll;
    f32 cosNegRoll;
    Vec3f spA4;
    f32 sp54;
    f32 sp4C;
    f32 cosPitchCosYawSinRoll;
    f32 negSinPitch;
    f32 temp_f10_2;
    f32 cosPitchcosYaw;
    f32 temp_f14;
    f32 negSinPitchSinYaw;
    f32 negSinPitchCosYaw;
    f32 cosPitchSinYaw;
    f32 temp_f4_2;
    f32 temp_f6;
    f32 temp_f8;
    f32 temp_f8_2;
    f32 temp_f8_3;
    
    sinPitch = SinS(pitch);
    cosPitch = CosS(pitch);
    sinYaw = SinS(yaw);
    cosYaw = CosS(yaw);
    negSinPitch = -sinPitch;
    sinNegRoll = SinS(-roll);
    cosNegRoll = CosS(-roll);
    negSinPitchSinYaw = negSinPitch * sinYaw;
    temp_f14 = 1.0f - cosNegRoll;
    cosPitchSinYaw = cosPitch * sinYaw;
    sp54 = SQ(cosPitchSinYaw);
    sp4C = (cosPitchSinYaw * sinPitch) * temp_f14;
    cosPitchcosYaw = cosPitch * cosYaw;
    temp_f4_2 = ((1.0f - sp54) * cosNegRoll) + sp54;
    cosPitchCosYawSinRoll = cosPitchcosYaw * sinNegRoll;
    negSinPitchCosYaw = negSinPitch * cosYaw;
    temp_f6 = (cosPitchcosYaw * cosPitchSinYaw) * temp_f14;
    temp_f10_2 = sinPitch * sinNegRoll;
    spA4.x = ((negSinPitchSinYaw * temp_f4_2) + (cosPitch * (sp4C - cosPitchCosYawSinRoll))) +
        (negSinPitchCosYaw * (temp_f6 + temp_f10_2));
    sp54 = SQ(sinPitch);
    temp_f4_2 = (sinPitch * cosPitchcosYaw) * temp_f14;
    temp_f8_3 = cosPitchSinYaw * sinNegRoll;
    temp_f8 = sp4C + cosPitchCosYawSinRoll;
    spA4.y = ((negSinPitchSinYaw * temp_f8) + (cosPitch * (((1.0f - sp54) * cosNegRoll) + sp54))) +
        (negSinPitchCosYaw * (temp_f4_2 - temp_f8_3));
    temp_f8_2 = temp_f6 - temp_f10_2;
    spA4.z = ((negSinPitchSinYaw * temp_f8_2) + (cosPitch * (temp_f4_2 + temp_f8_3))) +
        (negSinPitchCosYaw * (((1.0f - SQ(cosPitchcosYaw)) * cosNegRoll) + SQ(cosPitchcosYaw)));
    *dest = spA4;
    
    return dest;
}

// # # # # # # # # # # # # # # # # # # # #
// # Smooth                              #
// # # # # # # # # # # # # # # # # # # # #

static inline f32 Math_DelSmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep) {
    step *= gDeltaTime;
    minStep *= gDeltaTime;
    
    if (*pValue != target) {
        f32 stepSize = (target - *pValue) * fraction;
        
        if ((stepSize >= minStep) || (stepSize <= -minStep)) {
            if (stepSize > step) {
                stepSize = step;
            }
            
            if (stepSize < -step) {
                stepSize = -step;
            }
            
            *pValue += stepSize;
        } else {
            if (stepSize < minStep) {
                *pValue += minStep;
                stepSize = minStep;
                
                if (target < *pValue) {
                    *pValue = target;
                }
            }
            if (stepSize > -minStep) {
                *pValue += -minStep;
                
                if (*pValue < target) {
                    *pValue = target;
                }
            }
        }
    }
    
    return fabsf(target - *pValue);
}

static inline f64 Math_DelSmoothStepToD(f64* pValue, f64 target, f64 fraction, f64 step, f64 minStep) {
    step *= gDeltaTime;
    minStep *= gDeltaTime;
    
    if (*pValue != target) {
        f64 stepSize = (target - *pValue) * fraction;
        
        if ((stepSize >= minStep) || (stepSize <= -minStep))
            *pValue += clamp(stepSize, -step, step);
        
        else {
            if (stepSize < minStep) {
                *pValue += minStep;
                stepSize = minStep;
                
                if (target < *pValue)
                    *pValue = target;
            }
            if (stepSize > -minStep) {
                *pValue += -minStep;
                
                if (*pValue < target)
                    *pValue = target;
            }
        }
    }
    
    return fabs(target - *pValue);
}

static inline s16 Math_SmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep) {
    s16 stepSize = 0;
    s16 diff = target - *pValue;
    
    if (*pValue != target) {
        stepSize = diff / scale;
        
        if ((stepSize > minStep) || (stepSize < -minStep))
            *pValue += clamp(stepSize, -step, step);
        
        else {
            if (diff >= 0) {
                *pValue += minStep;
                
                if ((s16)(target - *pValue) <= 0)
                    *pValue = target;
            } else {
                *pValue -= minStep;
                
                if ((s16)(target - *pValue) >= 0)
                    *pValue = target;
            }
        }
    }
    
    return diff;
}

static inline int Math_SmoothStepToI(int* pValue, int target, int scale, int step, int minStep) {
    int stepSize = 0;
    int diff = target - *pValue;
    
    if (*pValue != target) {
        stepSize = diff / scale;
        
        if ((stepSize > minStep) || (stepSize < -minStep))
            *pValue += clamp(stepSize, -step, step);
        
        else {
            if (diff >= 0) {
                *pValue += minStep;
                
                if ((s16)(target - *pValue) <= 0)
                    *pValue = target;
            } else {
                *pValue -= minStep;
                
                if ((s16)(target - *pValue) >= 0)
                    *pValue = target;
            }
        }
    }
    
    return diff;
}

// # # # # # # # # # # # # # # # # # # # #
// # Rect                                #
// # # # # # # # # # # # # # # # # # # # #

static inline void Rect_ToCRect(CRect* dst, Rect* src) {
    if (dst) {
        dst->x1 = src->x;
        dst->y1 = src->y;
        dst->x2 = src->x + src->w;
        dst->y2 = src->y + src->h;
    }
}

static inline void Rect_ToRect(Rect* dst, CRect* src) {
    if (dst) {
        dst->x = src->x1;
        dst->y = src->y1;
        dst->w = src->x2 + src->x1;
        dst->h = src->y2 + src->y1;
    }
}

static inline bool Rect_Check_PosIntersect(Rect* rect, Vec2s* pos) {
    return !(
        pos->y < rect->y ||
        pos->y > rect->h ||
        pos->x < rect->x ||
        pos->x > rect->w
    );
}

static inline Rect Rect_Translate(Rect r, s32 x, s32 y) {
    r.x += x;
    r.w += x;
    r.y += y;
    r.h += y;
    
    return r;
}

static inline Rect Rect_New(s32 x, s32 y, s32 w, s32 h) {
    Rect dest = { x, y, w, h };
    
    return dest;
}

static inline Rect Rect_Add(Rect a, Rect b) {
    return (Rect) {
        a.x + b.x,
        a.y + b.y,
        a.w + b.w,
        a.h + b.h,
    };
}

static inline Rect Rect_Sub(Rect a, Rect b) {
    return (Rect) {
        a.x - b.x,
        a.y - b.y,
        a.w - b.w,
        a.h - b.h,
    };
}

static inline Rect Rect_AddPos(Rect a, Rect b) {
    return (Rect) {
        a.x + b.x,
        a.y + b.y,
        a.w,
        a.h,
    };
}

static inline Rect Rect_SubPos(Rect a, Rect b) {
    return (Rect) {
        a.x - b.x,
        a.y - b.y,
        a.w,
        a.h,
    };
}

static inline bool Rect_PointIntersect(Rect* rect, s32 x, s32 y) {
    if (x >= rect->x && x < rect->x + rect->w)
        if (y >= rect->y && y < rect->y + rect->h)
            return true;
    
    return false;
}

static inline Vec2s Rect_ClosestPoint(Rect* rect, s32 x, s32 y) {
    Vec2s r;
    
    if (x >= rect->x && x < rect->x + rect->w)
        r.x = x;
    
    else
        r.x = Closest(x, rect->x, rect->x + rect->w);
    
    if (y >= rect->y && y < rect->y + rect->h)
        r.y = y;
    
    else
        r.y = Closest(y, rect->y, rect->y + rect->h);
    
    return r;
}

static inline Vec2s Rect_MidPoint(Rect* rect) {
    return (Vec2s) {
        rect->x + rect->w * 0.5,
        rect->y + rect->h * 0.5
    };
}

static inline f32 Rect_PointDistance(Rect* rect, s32 x, s32 y) {
    Vec2s p = { x, y };
    Vec2s r = Rect_ClosestPoint(rect, x, y);
    
    return Math_Vec2s_DistXZ(r, p);
}

static inline int RectW(Rect r) {
    return r.x + r.w;
}

static inline int RectH(Rect r) {
    return r.y + r.h;
}

static inline Rect Rect_Clamp(Rect r, Rect l) {
    int diff;
    
    if ((diff = l.x - r.x) > 0) {
        r.x += diff;
        r.w -= diff;
    }
    
    if ((diff = l.y - r.y) > 0) {
        r.y += diff;
        r.h -= diff;
    }
    
    if ((diff = RectW(r) - RectW(l)) > 0) {
        r.w -= diff;
    }
    
    if ((diff = RectH(r) - RectH(l)) > 0) {
        r.h -= diff;
    }
    
    return r;
}

static inline Rect Rect_FlipHori(Rect r, Rect p) {
    r = Rect_SubPos(r, p);
    r.x = remapf(r.x, 0, p.w, p.w, 0) - r.w;
    return Rect_AddPos(r, p);
}

static inline Rect Rect_FlipVerti(Rect r, Rect p) {
    r = Rect_SubPos(r, p);
    r.y = remapf(r.y, 0, p.h, p.h, 0) - r.h;
    return Rect_AddPos(r, p);
}

static inline Rect Rect_ExpandX(Rect r, int amount) {
    if (amount < 0) {
        r.x -= abs(amount);
        r.w += abs(amount);
    } else {
        r.w += abs(amount);
    }
    
    return r;
}

static inline Rect Rect_ShrinkX(Rect r, int amount) {
    if (amount < 0) {
        r.w -= abs(amount);
    } else {
        r.x += abs(amount);
        r.w -= abs(amount);
    }
    
    return r;
}

static inline Rect Rect_ExpandY(Rect r, int amount) {
    amount = -amount;
    
    if (amount < 0) {
        r.y -= abs(amount);
        r.h += abs(amount);
    } else {
        r.h += abs(amount);
    }
    
    return r;
}

static inline Rect Rect_ShrinkY(Rect r, int amount) {
    amount = -amount;
    
    if (amount < 0) {
        r.h -= abs(amount);
    } else {
        r.y += abs(amount);
        r.h -= abs(amount);
    }
    
    return r;
}

static inline Rect Rect_Scale(Rect r, int x, int y) {
    x = -x;
    y = -y;
    
    r.x += (x);
    r.w -= (x) * 2;
    r.y += (y);
    r.h -= (y) * 2;
    
    return r;
}

static inline BoundBox BoundBox_New3F(Vec3f point) {
    BoundBox this;
    
    this.xMax = point.x;
    this.xMin = point.x;
    this.yMax = point.y;
    this.yMin = point.y;
    this.zMax = point.z;
    this.zMin = point.z;
    
    return this;
}

static inline BoundBox BoundBox_New2F(Vec2f point) {
    BoundBox this = {};
    
    this.xMax = point.x;
    this.xMin = point.x;
    this.yMax = point.y;
    this.yMin = point.y;
    
    return this;
}

static inline void BoundBox_Adjust3F(BoundBox* this, Vec3f point) {
    this->xMax = Max(this->xMax, point.x);
    this->xMin = Min(this->xMin, point.x);
    this->yMax = Max(this->yMax, point.y);
    this->yMin = Min(this->yMin, point.y);
    this->zMax = Max(this->zMax, point.z);
    this->zMin = Min(this->zMin, point.z);
}

static inline void BoundBox_Adjust2F(BoundBox* this, Vec2f point) {
    this->xMax = Max(this->xMax, point.x);
    this->xMin = Min(this->xMin, point.x);
    this->yMax = Max(this->yMax, point.y);
    this->yMin = Min(this->yMin, point.y);
}

static inline bool Math_Vec2f_PointInShape(Vec2f p, Vec2f* poly, u32 numPoly) {
    bool in = false;
    
    for (u32 i = 0, j = numPoly - 1; i < numPoly; j = i++) {
        if ((poly[i].y > p.y) != (poly[j].y > p.y)
            && p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x ) {
            in = !in;
        }
    }
    
    return in;
}

// # # # # # # # # # # # # # # # # # # # #
// # Vector                              #
// # # # # # # # # # # # # # # # # # # # #

static inline Vec3f Math_Vec3f_Cross(Vec3f a, Vec3f b) {
    return (Vec3f) {
        (a.y * b.z - b.y * a.z),
        (a.z * b.x - b.z * a.x),
        (a.x * b.y - b.x * a.y)
    };
}

static inline Vec3s Math_Vec3s_Cross(Vec3s a, Vec3s b) {
    return (Vec3s) {
        (a.y * b.z - b.y * a.z),
        (a.z * b.x - b.z * a.x),
        (a.x * b.y - b.x * a.y)
    };
}

static inline f32 Math_Vec3f_DistXZ(Vec3f a, Vec3f b) {
    f32 dx = b.x - a.x;
    f32 dz = b.z - a.z;
    
    return sqrtf(SQ(dx) + SQ(dz));
}

static inline f32 Math_Vec3f_DistXYZ(Vec3f a, Vec3f b) {
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    f32 dz = b.z - a.z;
    
    return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

static inline f32 Math_Vec3s_DistXZ(Vec3s a, Vec3s b) {
    f32 dx = b.x - a.x;
    f32 dz = b.z - a.z;
    
    return sqrtf(SQ(dx) + SQ(dz));
}

static inline f32 Math_Vec3s_DistXYZ(Vec3s a, Vec3s b) {
    f32 dx = b.x - a.x;
    f32 dy = b.y - a.y;
    f32 dz = b.z - a.z;
    
    return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

static inline f32 Math_Vec2f_DistXZ(Vec2f a, Vec2f b) {
    f32 dx = b.x - a.x;
    f32 dz = b.y - a.y;
    
    return sqrtf(SQ(dx) + SQ(dz));
}

static inline f32 Math_Vec2s_DistXZ(Vec2s a, Vec2s b) {
    f32 dx = b.x - a.x;
    f32 dz = b.y - a.y;
    
    return sqrtf(SQ(dx) + SQ(dz));
}

static inline s16 Math_Vec3f_Yaw(Vec3f a, Vec3f b) {
    f32 dx = b.x - a.x;
    f32 dz = b.z - a.z;
    
    return Atan2S(dz, dx);
}

static inline s16 Math_Vec2f_Yaw(Vec2f a, Vec2f b) {
    f32 dx = b.x - a.x;
    f32 dz = b.y - a.y;
    
    return Atan2S(dz, dx);
}

static inline s16 Math_Vec3f_Pitch(Vec3f a, Vec3f b) {
    return Atan2S(Math_Vec3f_DistXZ(a, b), a.y - b.y);
}

static inline Vec2f Math_Vec2f_Sub(Vec2f a, Vec2f b) {
    return (Vec2f) { a.x - b.x, a.y - b.y };
}

static inline Vec3f Math_Vec3f_Sub(Vec3f a, Vec3f b) {
    return (Vec3f) { a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec4f Math_Vec4f_Sub(Vec4f a, Vec4f b) {
    return (Vec4f) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

static inline Vec2f Math_Vec2f_SubVal(Vec2f a, f32 val) {
    return (Vec2f) { a.x - val, a.y - val };
}

static inline Vec3f Math_Vec3f_SubVal(Vec3f a, f32 val) {
    return (Vec3f) { a.x - val, a.y - val, a.z - val };
}

static inline Vec4f Math_Vec4f_SubVal(Vec4f a, f32 val) {
    return (Vec4f) { a.x - val, a.y - val, a.z - val, a.w - val };
}

static inline Vec2s Math_Vec2s_Sub(Vec2s a, Vec2s b) {
    return (Vec2s) { a.x - b.x, a.y - b.y };
}

static inline Vec3s Math_Vec3s_Sub(Vec3s a, Vec3s b) {
    return (Vec3s) { a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec4s Math_Vec4s_Sub(Vec4s a, Vec4s b) {
    return (Vec4s) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

static inline Vec2s Math_Vec2s_SubVal(Vec2s a, s16 val) {
    return (Vec2s) { a.x - val, a.y - val };
}

static inline Vec3s Math_Vec3s_SubVal(Vec3s a, s16 val) {
    return (Vec3s) { a.x - val, a.y - val, a.z - val };
}

static inline Vec4s Math_Vec4s_SubVal(Vec4s a, s16 val) {
    return (Vec4s) { a.x - val, a.y - val, a.z - val, a.w - val };
}

static inline Vec2f Math_Vec2f_Add(Vec2f a, Vec2f b) {
    return (Vec2f) { a.x + b.x, a.y + b.y };
}

static inline Vec3f Math_Vec3f_Add(Vec3f a, Vec3f b) {
    return (Vec3f) { a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vec4f Math_Vec4f_Add(Vec4f a, Vec4f b) {
    return (Vec4f) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

static inline Vec2f Math_Vec2f_AddVal(Vec2f a, f32 val) {
    return (Vec2f) { a.x + val, a.y + val };
}

static inline Vec3f Math_Vec3f_AddVal(Vec3f a, f32 val) {
    return (Vec3f) { a.x + val, a.y + val, a.z + val };
}

static inline Vec4f Math_Vec4f_AddVal(Vec4f a, f32 val) {
    return (Vec4f) { a.x + val, a.y + val, a.z + val, a.w + val };
}

static inline Vec2s Math_Vec2s_Add(Vec2s a, Vec2s b) {
    return (Vec2s) { a.x + b.x, a.y + b.y };
}

static inline Vec3s Math_Vec3s_Add(Vec3s a, Vec3s b) {
    return (Vec3s) { a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vec4s Math_Vec4s_Add(Vec4s a, Vec4s b) {
    return (Vec4s) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

static inline Vec2s Math_Vec2s_AddVal(Vec2s a, s16 val) {
    return (Vec2s) { a.x + val, a.y + val };
}

static inline Vec3s Math_Vec3s_AddVal(Vec3s a, s16 val) {
    return (Vec3s) { a.x + val, a.y + val, a.z + val };
}

static inline Vec4s Math_Vec4s_AddVal(Vec4s a, s16 val) {
    return (Vec4s) { a.x + val, a.y + val, a.z + val, a.w + val };
}

static inline Vec2f Math_Vec2f_Div(Vec2f a, Vec2f b) {
    return (Vec2f) { a.x / b.x, a.y / b.y };
}

static inline Vec3f Math_Vec3f_Div(Vec3f a, Vec3f b) {
    return (Vec3f) { a.x / b.x, a.y / b.y, a.z / b.z };
}

static inline Vec4f Math_Vec4f_Div(Vec4f a, Vec4f b) {
    return (Vec4f) { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

static inline Vec2f Math_Vec2f_DivVal(Vec2f a, f32 val) {
    return (Vec2f) { a.x / val, a.y / val };
}

static inline Vec3f Math_Vec3f_DivVal(Vec3f a, f32 val) {
    return (Vec3f) { a.x / val, a.y / val, a.z / val };
}

static inline Vec4f Math_Vec4f_DivVal(Vec4f a, f32 val) {
    return (Vec4f) { a.x / val, a.y / val, a.z / val, a.w / val };
}

static inline Vec2s Math_Vec2s_Div(Vec2s a, Vec2s b) {
    return (Vec2s) { a.x / b.x, a.y / b.y };
}

static inline Vec3s Math_Vec3s_Div(Vec3s a, Vec3s b) {
    return (Vec3s) { a.x / b.x, a.y / b.y, a.z / b.z };
}

static inline Vec4s Math_Vec4s_Div(Vec4s a, Vec4s b) {
    return (Vec4s) { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

static inline Vec2s Math_Vec2s_DivVal(Vec2s a, f32 val) {
    return (Vec2s) { a.x / val, a.y / val };
}

static inline Vec3s Math_Vec3s_DivVal(Vec3s a, f32 val) {
    return (Vec3s) { a.x / val, a.y / val, a.z / val };
}

static inline Vec4s Math_Vec4s_DivVal(Vec4s a, f32 val) {
    return (Vec4s) { a.x / val, a.y / val, a.z / val, a.w / val };
}

static inline Vec2f Math_Vec2f_Mul(Vec2f a, Vec2f b) {
    return (Vec2f) { a.x* b.x, a.y* b.y };
}

static inline Vec3f Math_Vec3f_Mul(Vec3f a, Vec3f b) {
    return (Vec3f) { a.x* b.x, a.y* b.y, a.z* b.z };
}

static inline Vec4f Math_Vec4f_Mul(Vec4f a, Vec4f b) {
    return (Vec4f) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

static inline Vec2f Math_Vec2f_MulVal(Vec2f a, f32 val) {
    return (Vec2f) { a.x* val, a.y* val };
}

static inline Vec3f Math_Vec3f_MulVal(Vec3f a, f32 val) {
    return (Vec3f) { a.x* val, a.y* val, a.z* val };
}

static inline Vec4f Math_Vec4f_MulVal(Vec4f a, f32 val) {
    return (Vec4f) { a.x* val, a.y* val, a.z* val, a.w* val };
}

static inline Vec2s Math_Vec2s_Mul(Vec2s a, Vec2s b) {
    return (Vec2s) { a.x* b.x, a.y* b.y };
}

static inline Vec3s Math_Vec3s_Mul(Vec3s a, Vec3s b) {
    return (Vec3s) { a.x* b.x, a.y* b.y, a.z* b.z };
}

static inline Vec4s Math_Vec4s_Mul(Vec4s a, Vec4s b) {
    return (Vec4s) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

static inline Vec2s Math_Vec2s_MulVal(Vec2s a, f32 val) {
    return (Vec2s) { a.x* val, a.y* val };
}

static inline Vec3s Math_Vec3s_MulVal(Vec3s a, f32 val) {
    return (Vec3s) { a.x* val, a.y* val, a.z* val };
}

static inline Vec4s Math_Vec4s_MulVal(Vec4s a, f32 val) {
    return (Vec4s) { a.x* val, a.y* val, a.z* val, a.w* val };
}

static inline Vec2f Math_Vec2f_New(f32 x, f32 y) {
    return (Vec2f) { x, y };
}

static inline Vec3f Math_Vec3f_New(f32 x, f32 y, f32 z) {
    return (Vec3f) { x, y, z };
}

static inline Vec4f Math_Vec4f_New(f32 x, f32 y, f32 z, f32 w) {
    return (Vec4f) { x, y, z, w };
}

static inline Vec2s Math_Vec2s_New(s16 x, s16 y) {
    return (Vec2s) { x, y };
}

static inline Vec3s Math_Vec3s_New(s16 x, s16 y, s16 z) {
    return (Vec3s) { x, y, z };
}

static inline Vec4s Math_Vec4s_New(s16 x, s16 y, s16 z, s16 w) {
    return (Vec4s) { x, y, z, w };
}

static inline f32 Math_Vec2f_Dot(Vec2f a, Vec2f b) {
    f32 dot = 0.0f;
    
    for (int i = 0; i < 2; i++)
        dot += a.axis[i] * b.axis[i];
    
    return dot;
}

static inline f32 Math_Vec3f_Dot(Vec3f a, Vec3f b) {
    f32 dot = 0.0f;
    
    for (int i = 0; i < 3; i++)
        dot += a.axis[i] * b.axis[i];
    
    return dot;
}

static inline f32 Math_Vec4f_Dot(Vec4f a, Vec4f b) {
    f32 dot = 0.0f;
    
    for (int i = 0; i < 4; i++)
        dot += a.axis[i] * b.axis[i];
    
    return dot;
}

static inline f32 Math_Vec2s_Dot(Vec2s a, Vec2s b) {
    f32 dot = 0.0f;
    
    for (int i = 0; i < 2; i++)
        dot += a.axis[i] * b.axis[i];
    
    return dot;
}

static inline f32 Math_Vec3s_Dot(Vec3s a, Vec3s b) {
    f32 dot = 0.0f;
    
    for (int i = 0; i < 3; i++)
        dot += a.axis[i] * b.axis[i];
    
    return dot;
}

static inline f32 Math_Vec4s_Dot(Vec4s a, Vec4s b) {
    f32 dot = 0.0f;
    
    for (int i = 0; i < 4; i++)
        dot += a.axis[i] * b.axis[i];
    
    return dot;
}

static inline f32 Math_Vec2f_MagnitudeSQ(Vec2f a) {
    return Math_Vec2f_Dot(a, a);
}

static inline f32 Math_Vec3f_MagnitudeSQ(Vec3f a) {
    return Math_Vec3f_Dot(a, a);
}

static inline f32 Math_Vec4f_MagnitudeSQ(Vec4f a) {
    return Math_Vec4f_Dot(a, a);
}

static inline f32 Math_Vec2s_MagnitudeSQ(Vec2s a) {
    return Math_Vec2s_Dot(a, a);
}

static inline f32 Math_Vec3s_MagnitudeSQ(Vec3s a) {
    return Math_Vec3s_Dot(a, a);
}

static inline f32 Math_Vec4s_MagnitudeSQ(Vec4s a) {
    return Math_Vec4s_Dot(a, a);
}

static inline f32 Math_Vec2f_Magnitude(Vec2f a) {
    return sqrtf(Math_Vec2f_MagnitudeSQ(a));
}

static inline f32 Math_Vec3f_Magnitude(Vec3f a) {
    return sqrtf(Math_Vec3f_MagnitudeSQ(a));
}

static inline f32 Math_Vec4f_Magnitude(Vec4f a) {
    return sqrtf(Math_Vec4f_MagnitudeSQ(a));
}

static inline f32 Math_Vec2s_Magnitude(Vec2s a) {
    return sqrtf(Math_Vec2s_MagnitudeSQ(a));
}

static inline f32 Math_Vec3s_Magnitude(Vec3s a) {
    return sqrtf(Math_Vec3s_MagnitudeSQ(a));
}

static inline f32 Math_Vec4s_Magnitude(Vec4s a) {
    return sqrtf(Math_Vec4s_MagnitudeSQ(a));
}

static inline Vec2f Math_Vec2f_Median(Vec2f a, Vec2f b) {
    Vec2f vec;
    
    for (int i = 0; i < 2; i++)
        vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
    
    return vec;
}

static inline Vec3f Math_Vec3f_Median(Vec3f a, Vec3f b) {
    Vec3f vec;
    
    for (int i = 0; i < 3; i++)
        vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
    
    return vec;
}

static inline Vec4f Math_Vec4f_Median(Vec4f a, Vec4f b) {
    Vec4f vec;
    
    for (int i = 0; i < 4; i++)
        vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
    
    return vec;
}

static inline Vec2s Math_Vec2s_Median(Vec2s a, Vec2s b) {
    Vec2s vec;
    
    for (int i = 0; i < 2; i++)
        vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
    
    return vec;
}

static inline Vec3s Math_Vec3s_Median(Vec3s a, Vec3s b) {
    Vec3s vec;
    
    for (int i = 0; i < 3; i++)
        vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
    
    return vec;
}

static inline Vec4s Math_Vec4s_Median(Vec4s a, Vec4s b) {
    Vec4s vec;
    
    for (int i = 0; i < 4; i++)
        vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
    
    return vec;
}

static inline Vec2f Math_Vec2f_Normalize(Vec2f a) {
    f32 mgn = Math_Vec2f_Magnitude(a);
    
    if (mgn == 0.0f)
        return Math_Vec2f_MulVal(a, 0.0f);
    else
        return Math_Vec2f_DivVal(a, mgn);
}

static inline Vec3f Math_Vec3f_Normalize(Vec3f a) {
    f32 mgn = Math_Vec3f_Magnitude(a);
    
    if (mgn == 0.0f)
        return Math_Vec3f_MulVal(a, 0.0f);
    else
        return Math_Vec3f_DivVal(a, mgn);
}

static inline Vec4f Math_Vec4f_Normalize(Vec4f a) {
    f32 mgn = Math_Vec4f_Magnitude(a);
    
    if (mgn == 0.0f)
        return Math_Vec4f_MulVal(a, 0.0f);
    else
        return Math_Vec4f_DivVal(a, mgn);
}

static inline Vec2s Math_Vec2s_Normalize(Vec2s a) {
    f32 mgn = Math_Vec2s_Magnitude(a);
    
    if (mgn == 0.0f)
        return Math_Vec2s_MulVal(a, 0.0f);
    else
        return Math_Vec2s_DivVal(a, mgn);
}

static inline Vec3s Math_Vec3s_Normalize(Vec3s a) {
    f32 mgn = Math_Vec3s_Magnitude(a);
    
    if (mgn == 0.0f)
        return Math_Vec3s_MulVal(a, 0.0f);
    else
        return Math_Vec3s_DivVal(a, mgn);
}

static inline Vec4s Math_Vec4s_Normalize(Vec4s a) {
    f32 mgn = Math_Vec4s_Magnitude(a);
    
    if (mgn == 0.0f)
        return Math_Vec4s_MulVal(a, 0.0f);
    else
        return Math_Vec4s_DivVal(a, mgn);
}

static inline Vec2f Math_Vec2f_LineSegDir(Vec2f a, Vec2f b) {
    return Math_Vec2f_Normalize(Math_Vec2f_Sub(b, a));
}

static inline Vec3f Math_Vec3f_LineSegDir(Vec3f a, Vec3f b) {
    return Math_Vec3f_Normalize(Math_Vec3f_Sub(b, a));
}

static inline Vec4f Math_Vec4f_LineSegDir(Vec4f a, Vec4f b) {
    return Math_Vec4f_Normalize(Math_Vec4f_Sub(b, a));
}

static inline Vec2s Math_Vec2s_LineSegDir(Vec2s a, Vec2s b) {
    return Math_Vec2s_Normalize(Math_Vec2s_Sub(b, a));
}

static inline Vec3s Math_Vec3s_LineSegDir(Vec3s a, Vec3s b) {
    return Math_Vec3s_Normalize(Math_Vec3s_Sub(b, a));
}

static inline Vec4s Math_Vec4s_LineSegDir(Vec4s a, Vec4s b) {
    return Math_Vec4s_Normalize(Math_Vec4s_Sub(b, a));
}

static inline Vec2f Math_Vec2f_Project(Vec2f a, Vec2f b) {
    f32 ls = Math_Vec2f_MagnitudeSQ(b);
    
    return Math_Vec2f_MulVal(b, Math_Vec2f_Dot(b, a) / ls);
}

static inline Vec3f Math_Vec3f_Project(Vec3f a, Vec3f b) {
    f32 ls = Math_Vec3f_MagnitudeSQ(b);
    
    return Math_Vec3f_MulVal(b, Math_Vec3f_Dot(b, a) / ls);
}

static inline Vec4f Math_Vec4f_Project(Vec4f a, Vec4f b) {
    f32 ls = Math_Vec4f_MagnitudeSQ(b);
    
    return Math_Vec4f_MulVal(b, Math_Vec4f_Dot(b, a) / ls);
}

static inline Vec2s Math_Vec2s_Project(Vec2s a, Vec2s b) {
    f32 ls = Math_Vec2s_MagnitudeSQ(b);
    
    return Math_Vec2s_MulVal(b, Math_Vec2s_Dot(b, a) / ls);
}

static inline Vec3s Math_Vec3s_Project(Vec3s a, Vec3s b) {
    f32 ls = Math_Vec3s_MagnitudeSQ(b);
    
    return Math_Vec3s_MulVal(b, Math_Vec3s_Dot(b, a) / ls);
}

static inline Vec4s Math_Vec4s_Project(Vec4s a, Vec4s b) {
    f32 ls = Math_Vec4s_MagnitudeSQ(b);
    
    return Math_Vec4s_MulVal(b, Math_Vec4s_Dot(b, a) / ls);
}

static inline Vec2f Math_Vec2f_Invert(Vec2f a) {
    return Math_Vec2f_MulVal(a, -1);
}

static inline Vec3f Math_Vec3f_Invert(Vec3f a) {
    return Math_Vec3f_MulVal(a, -1);
}

static inline Vec4f Math_Vec4f_Invert(Vec4f a) {
    return Math_Vec4f_MulVal(a, -1);
}

static inline Vec2s Math_Vec2s_Invert(Vec2s a) {
    return Math_Vec2s_MulVal(a, -1);
}

static inline Vec3s Math_Vec3s_Invert(Vec3s a) {
    return Math_Vec3s_MulVal(a, -1);
}

static inline Vec4s Math_Vec4s_Invert(Vec4s a) {
    return Math_Vec4s_MulVal(a, -1);
}

static inline Vec2f Math_Vec2f_InvMod(Vec2f a) {
    Vec2f r;
    
    for (int i = 0; i < ArrCount(a.axis); i++)
        r.axis[i] = 1.0f - fabsf(a.axis[i]);
    
    return r;
}

static inline Vec3f Math_Vec3f_InvMod(Vec3f a) {
    Vec3f r;
    
    for (int i = 0; i < ArrCount(a.axis); i++)
        r.axis[i] = 1.0f - fabsf(a.axis[i]);
    
    return r;
}

static inline Vec4f Math_Vec4f_InvMod(Vec4f a) {
    Vec4f r;
    
    for (int i = 0; i < ArrCount(a.axis); i++)
        r.axis[i] = 1.0f - fabsf(a.axis[i]);
    
    return r;
}

static inline bool Math_Vec2f_IsNaN(Vec2f a) {
    for (int i = 0; i < ArrCount(a.axis); i++)
        if (isnan(a.axis[i])) true;
    
    return false;
}

static inline bool Math_Vec3f_IsNaN(Vec3f a) {
    for (int i = 0; i < ArrCount(a.axis); i++)
        if (isnan(a.axis[i])) true;
    
    return false;
}

static inline bool Math_Vec4f_IsNaN(Vec4f a) {
    for (int i = 0; i < ArrCount(a.axis); i++)
        if (isnan(a.axis[i])) true;
    
    return false;
}

static inline f32 Math_Vec2f_Cos(Vec2f a, Vec2f b) {
    f32 mp = Math_Vec2f_Magnitude(a) * Math_Vec2f_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Math_Vec2f_Dot(a, b) / mp;
}

static inline f32 Math_Vec3f_Cos(Vec3f a, Vec3f b) {
    f32 mp = Math_Vec3f_Magnitude(a) * Math_Vec3f_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Math_Vec3f_Dot(a, b) / mp;
}

static inline f32 Math_Vec4f_Cos(Vec4f a, Vec4f b) {
    f32 mp = Math_Vec4f_Magnitude(a) * Math_Vec4f_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Math_Vec4f_Dot(a, b) / mp;
}

static inline f32 Math_Vec2s_Cos(Vec2s a, Vec2s b) {
    f32 mp = Math_Vec2s_Magnitude(a) * Math_Vec2s_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Math_Vec2s_Dot(a, b) / mp;
}

static inline f32 Math_Vec3s_Cos(Vec3s a, Vec3s b) {
    f32 mp = Math_Vec3s_Magnitude(a) * Math_Vec3s_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Math_Vec3s_Dot(a, b) / mp;
}

static inline f32 Math_Vec4s_Cos(Vec4s a, Vec4s b) {
    f32 mp = Math_Vec4s_Magnitude(a) * Math_Vec4s_Magnitude(b);
    
    if (IsZero(mp)) return 0.0f;
    
    return Math_Vec4s_Dot(a, b) / mp;
}

static inline Vec2f Math_Vec2f_Reflect(Vec2f vec, Vec2f normal) {
    Vec2f negVec = Math_Vec2f_Invert(vec);
    f32 vecDotNormal = Math_Vec2f_Cos(negVec, normal);
    Vec2f normalScale = Math_Vec2f_MulVal(normal, vecDotNormal);
    Vec2f nsVec = Math_Vec2f_Add(normalScale, vec);
    
    return Math_Vec2f_Add(negVec, Math_Vec2f_MulVal(nsVec, 2.0f));
}

static inline Vec3f Math_Vec3f_Reflect(Vec3f vec, Vec3f normal) {
    Vec3f negVec = Math_Vec3f_Invert(vec);
    f32 vecDotNormal = Math_Vec3f_Cos(negVec, normal);
    Vec3f normalScale = Math_Vec3f_MulVal(normal, vecDotNormal);
    Vec3f nsVec = Math_Vec3f_Add(normalScale, vec);
    
    return Math_Vec3f_Add(negVec, Math_Vec3f_MulVal(nsVec, 2.0f));
}

static inline Vec4f Math_Vec4f_Reflect(Vec4f vec, Vec4f normal) {
    Vec4f negVec = Math_Vec4f_Invert(vec);
    f32 vecDotNormal = Math_Vec4f_Cos(negVec, normal);
    Vec4f normalScale = Math_Vec4f_MulVal(normal, vecDotNormal);
    Vec4f nsVec = Math_Vec4f_Add(normalScale, vec);
    
    return Math_Vec4f_Add(negVec, Math_Vec4f_MulVal(nsVec, 2.0f));
}

static inline Vec2s Math_Vec2s_Reflect(Vec2s vec, Vec2s normal) {
    Vec2s negVec = Math_Vec2s_Invert(vec);
    f32 vecDotNormal = Math_Vec2s_Cos(negVec, normal);
    Vec2s normalScale = Math_Vec2s_MulVal(normal, vecDotNormal);
    Vec2s nsVec = Math_Vec2s_Add(normalScale, vec);
    
    return Math_Vec2s_Add(negVec, Math_Vec2s_MulVal(nsVec, 2.0f));
}

static inline Vec3s Math_Vec3s_Reflect(Vec3s vec, Vec3s normal) {
    Vec3s negVec = Math_Vec3s_Invert(vec);
    f32 vecDotNormal = Math_Vec3s_Cos(negVec, normal);
    Vec3s normalScale = Math_Vec3s_MulVal(normal, vecDotNormal);
    Vec3s nsVec = Math_Vec3s_Add(normalScale, vec);
    
    return Math_Vec3s_Add(negVec, Math_Vec3s_MulVal(nsVec, 2.0f));
}

static inline Vec4s Math_Vec4s_Reflect(Vec4s vec, Vec4s normal) {
    Vec4s negVec = Math_Vec4s_Invert(vec);
    f32 vecDotNormal = Math_Vec4s_Cos(negVec, normal);
    Vec4s normalScale = Math_Vec4s_MulVal(normal, vecDotNormal);
    Vec4s nsVec = Math_Vec4s_Add(normalScale, vec);
    
    return Math_Vec4s_Add(negVec, Math_Vec4s_MulVal(nsVec, 2.0f));
}

static inline Vec3f Math_Vec3f_ClosestPointOnRay(Vec3f rayStart, Vec3f rayEnd, Vec3f lineStart, Vec3f lineEnd) {
    Vec3f rayNorm = Math_Vec3f_LineSegDir(rayStart, rayEnd);
    Vec3f lineNorm = Math_Vec3f_LineSegDir(lineStart, lineEnd);
    f32 ndot = Math_Vec3f_Dot(rayNorm, lineNorm);
    
    if (fabsf(ndot) >= 1.0f - EPSILON) {
        // parallel
        
        return lineStart;
    } else if (IsZero(ndot)) {
        // perpendicular
        Vec3f mod = Math_Vec3f_InvMod(rayNorm);
        Vec3f ls = Math_Vec3f_Mul(lineStart, mod);
        Vec3f rs = Math_Vec3f_Mul(rayStart, mod);
        Vec3f side = Math_Vec3f_Project(Math_Vec3f_Sub(rs, ls), lineNorm);
        
        return Math_Vec3f_Add(lineStart, side);
    } else {
        Vec3f startDiff = Math_Vec3f_Sub(lineStart, rayStart);
        Vec3f crossNorm = Math_Vec3f_Normalize(Math_Vec3f_Cross(lineNorm, rayNorm));
        Vec3f rejection = Math_Vec3f_Sub(
            Math_Vec3f_Sub(startDiff, Math_Vec3f_Project(startDiff, rayNorm)),
            Math_Vec3f_Project(startDiff, crossNorm)
        );
        f32 rejDot = Math_Vec3f_Dot(lineNorm, Math_Vec3f_Normalize(rejection));
        f32 distToLinePos;
        
        if (rejDot == 0.0f)
            return lineStart;
        
        distToLinePos = Math_Vec3f_Magnitude(rejection) / rejDot;
        
        return Math_Vec3f_Sub(lineStart, Math_Vec3f_MulVal(lineNorm, distToLinePos));
    }
}

static inline Vec3f Math_Vec3f_ProjectAlong(Vec3f point, Vec3f lineA, Vec3f lineB) {
    Vec3f seg = Math_Vec3f_LineSegDir(lineA, lineB);
    Vec3f proj = Math_Vec3f_Project(Math_Vec3f_Sub(point, lineA), seg);
    
    return Math_Vec3f_Add(lineA, proj);
}

#pragma GCC diagnostic pop

#endif