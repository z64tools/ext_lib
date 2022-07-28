#ifndef __Z64_MATH_H__
#define __Z64_MATH_H__

#include <ExtLib.h>

extern f64 gDeltaTime;

#define VEC_TYPE(type, suffix) \
	typedef union { \
		struct { \
			type x; \
			type y; \
			type z; \
			type w; \
		}; \
		type axis[4]; \
	} Vec4 ## suffix; \
	typedef union { \
		struct { \
			type x; \
			type y; \
			type z; \
		}; \
		type axis[3]; \
	} Vec3 ## suffix; \
	typedef union { \
		struct { \
			type x; \
			type y; \
		}; \
		type axis[2]; \
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

// CoordinatesRect
typedef struct {
	s32 x1;
	s32 y1;
	s32 x2;
	s32 y2;
} CRect;

s16 Atan2S(f32 x, f32 y);
void Math_VecSphToVec3f(Vec3f* dest, VecSph* sph);
void Math_AddVecSphToVec3f(Vec3f* dest, VecSph* sph);
VecSph* Math_Vec3fToVecSph(VecSph* dest, Vec3f* vec);
VecSph* Math_Vec3fToVecSphGeo(VecSph* dest, Vec3f* vec);
VecSph* Math_Vec3fDiffToVecSphGeo(VecSph* dest, Vec3f* a, Vec3f* b);
Vec3f* Math_CalcUpFromPitchYawRoll(Vec3f* dest, s16 pitch, s16 yaw, s16 roll);
f32 Math_DelSmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
f64 Math_DelSmoothStepToD(f64* pValue, f64 target, f64 fraction, f64 step, f64 minStep);
s16 Math_DelSmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep);

void Rect_ToCRect(CRect* dst, Rect* src);
void Rect_ToRect(Rect* dst, CRect* src);
bool Rect_Check_PosIntersect(Rect* rect, Vec2s* pos);
void Rect_Translate(Rect* rect, s32 x, s32 y);
Rect Rect_New(s32 x, s32 y, s32 w, s32 h);
Rect Rect_Add(Rect a, Rect b);
Rect Rect_Sub(Rect a, Rect b);
Rect Rect_AddPos(Rect a, Rect b);
Rect Rect_SubPos(Rect a, Rect b);
bool Rect_PointIntersect(Rect* rect, s32 x, s32 y);
Vec2s Rect_ClosestPoint(Rect* rect, s32 x, s32 y);
Vec2s Rect_MidPoint(Rect* rect);
f32 Rect_PointDistance(Rect* rect, s32 x, s32 y);

#define veccmp(a, b) ({ \
		bool r = false; \
		for (s32 i = 0; i < sizeof((*(a))) / sizeof((*(a)).x); i++) \
		if ((*(a)).axis[i] != (*(b)).axis[i]) r = true; \
		r; \
	})

#define veccpy(a, b) do { \
	for (s32 i = 0; i < sizeof((*(a))) / sizeof((*(a)).x); i++) \
	(*(a)).axis[i] = (*(b)).axis[i]; \
} while (0)

#define SQ(x)   ((x) * (x))
#define SinS(x) sinf(BinToRad((s16)(x)))
#define CosS(x) cosf(BinToRad((s16)(x)))
#define SinF(x) sinf(DegToRad(x))
#define CosF(x) cosf(DegToRad(x))
#define SinR(x) sinf(x)
#define CosR(x) cosf(x)

// Trig macros
#define DegToBin(degreesf) (s16)(degreesf * 182.04167f + .5f)
#define RadToBin(radf)     (s16)(radf * (32768.0f / M_PI))
#define RadToDeg(radf)     (radf * (180.0f / M_PI))
#define DegToRad(degf)     (degf * (M_PI / 180.0f))
#define BinFlip(angle)     ((s16)(angle - 0x7FFF))
#define BinSub(a, b)       ((s16)(a - b))
#define BinToDeg(binang)   ((f32)binang * (360.0001525f / 65535.0f))
#define BinToRad(binang)   (((f32)binang / 32768.0f) * M_PI)

#define UnfoldRect(rect) (rect).x, (rect).y, (rect).w, (rect).h

#ifndef __VECTOR_H__
#include "Vector.h"
#endif

#endif
