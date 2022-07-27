#ifndef __Z64VECTOR_H__
#define __Z64VECTOR_H__

#include <ExtLib.h>

extern f64 gDeltaTime;

#define __EXT_VECTYPEDEF(type, suffix) \
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

__EXT_VECTYPEDEF(f32, f)
// __EXT_VECTYPEDEF(s32, i)
__EXT_VECTYPEDEF(s16, s)

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
f32 Math_Vec3f_DistXZ(Vec3f* a, Vec3f* b);
f32 Math_Vec3f_DistXYZ(Vec3f* a, Vec3f* b);
f32 Math_Vec3s_DistXZ(Vec3s* a, Vec3s* b);
f32 Math_Vec3s_DistXYZ(Vec3s* a, Vec3s* b);
f32 Math_Vec2f_DistXZ(Vec2f* a, Vec2f* b);
f32 Math_Vec2s_DistXZ(Vec2s* a, Vec2s* b);
s16 Math_Vec3f_Yaw(Vec3f* a, Vec3f* b);
s16 Math_Vec2f_Yaw(Vec2f* a, Vec2f* b);
s16 Math_Vec3f_Pitch(Vec3f* a, Vec3f* b);
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
void Rect_Verify(Rect* rect);
void Rect_Set(Rect* dest, s32 x, s32 w, s32 y, s32 h);
Rect Rect_Add(Rect* a, Rect* b);
Rect Rect_Sub(Rect* a, Rect* b);
Rect Rect_AddPos(Rect* a, Rect* b);
Rect Rect_SubPos(Rect* a, Rect* b);
bool Rect_PointIntersect(Rect* rect, s32 x, s32 y);
Vec2s Rect_ClosestPoint(Rect* rect, s32 x, s32 y);
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

#define Vector_New(format, type) \
	Vec2 ## format Math_Vec2 ## format ## _New(type x, type y); \
	Vec3 ## format Math_Vec3 ## format ## _New(type x, type y, type z); \
	Vec4 ## format Math_Vec4 ## format ## _New(type x, type y, type z, type w)

#define Vector_TwoArgRet(ret, name, format, type) \
	ret Math_Vec2 ## format ## _ ## name(Vec2 ## format a, Vec2 ## format b); \
	ret Math_Vec3 ## format ## _ ## name(Vec3 ## format a, Vec3 ## format b); \
	ret Math_Vec4 ## format ## _ ## name(Vec4 ## format a, Vec4 ## format b)

#define Vector_OneArgRet(ret, name, format, type) \
	ret Math_Vec2 ## format ## _ ## name(Vec2 ## format a); \
	ret Math_Vec3 ## format ## _ ## name(Vec3 ## format a); \
	ret Math_Vec4 ## format ## _ ## name(Vec4 ## format a)

#define Vector_Math(name, format, type) \
	Vec2 ## format Math_Vec2 ## format ## _ ## name(Vec2 ## format a, Vec2 ## format b); \
	Vec3 ## format Math_Vec3 ## format ## _ ## name(Vec3 ## format a, Vec3 ## format b); \
	Vec4 ## format Math_Vec4 ## format ## _ ## name(Vec4 ## format a, Vec4 ## format b); \
	Vec2 ## format Math_Vec2 ## format ## _ ## name ## Val(Vec2 ## format a, type val); \
	Vec3 ## format Math_Vec3 ## format ## _ ## name ## Val(Vec3 ## format a, type val); \
	Vec4 ## format Math_Vec4 ## format ## _ ## name ## Val(Vec4 ## format a, type val)

#define Vector_VecValRetSame(name, format, type) \
	Vec2 ## format Math_Vec2 ## format ## _ ## name(Vec2 ## format a, type val); \
	Vec3 ## format Math_Vec3 ## format ## _ ## name(Vec3 ## format a, type val); \
	Vec4 ## format Math_Vec4 ## format ## _ ## name(Vec4 ## format a, type val)

Vector_New(f, f32);
// Vector_New(i, s32);
Vector_New(s, s16);

Vector_Math(Add, f, f32);
// Vector_Math(Add, i, s32);
Vector_Math(Add, s, s16);
Vector_Math(Sub, f, f32);
// Vector_Math(Sub, i, s32);
Vector_Math(Sub, s, s16);
Vector_Math(Div, f, f32);
// Vector_Math(Div, i, s32);
Vector_Math(Div, s, s16);
Vector_Math(Mul, f, f32);
// Vector_Math(Mul, i, s32);
Vector_Math(Mul, s, s16);
Vector_TwoArgRet(bool, Equal, f, f32);
// Vector_TwoArgRet(bool, Equal, i, s32);
Vector_TwoArgRet(bool, Equal, s, s16);
Vector_TwoArgRet(f32, Dot, f, f32);
// Vector_TwoArgRet(f32, Dot, i, s32);
Vector_TwoArgRet(f32, Dot, s, s16);
Vector_OneArgRet(f32, MagnitudeSQ, f, f32);
// Vector_OneArgRet(f32, MagnitudeSQ, i, s32);
Vector_OneArgRet(f32, MagnitudeSQ, s, s16);
Vector_OneArgRet(f32, Magnitude, f, f32);
// Vector_OneArgRet(f32, Magnitude, i, s32);
Vector_OneArgRet(f32, Magnitude, s, s16);
Vector_OneArgRet(void, Normalize, f, f32);
// Vector_OneArgRet(void, Normalize, i, s32);
Vector_OneArgRet(void, Normalize, s, s16);

Vec3f Vec3f_Cross(Vec3f a, Vec3f b);
// Vec3i Vec3i_Cross(Vec3i a, Vec3i b);
Vec3s Vec3s_Cross(Vec3s a, Vec3s b);

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

#endif
