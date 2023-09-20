#ifndef EXT_VECTOR_H
#define EXT_VECTOR_H

#include "ext_type.h"
#include "ext_math.h"
#include "ext_macros.h"
#include <math.h>

#define veccmp(a, b) ({ \
		bool r = false; \
		for (int i = 0; i < sizeof((*(a))) / sizeof((*(a)).x); i++) \
		if ((*(a)).axis[i] != (*(b)).axis[i]) r = true; \
		r; \
	})

#define veccpy(a, b) do { \
	for (int i = 0; i < sizeof((*(a))) / sizeof((*(a)).x); i++) \
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

typedef struct {
	f32 x; f32 y; f32 z; f32 w;
} Quat;

extern const f32 EPSILON;
extern f32 gDeltaTime;

s16 Atan2S(f32 x, f32 y);
void VecSphToVec3f(Vec3f* dest, VecSph* sph);
void Math_AddVecSphToVec3f(Vec3f* dest, VecSph* sph);
VecSph* VecSph_FromVec3f(VecSph* dest, Vec3f* vec);
VecSph* VecSph_GeoFromVec3f(VecSph* dest, Vec3f* vec);
VecSph* VecSph_GeoFromVec3fDiff(VecSph* dest, Vec3f* a, Vec3f* b);
Vec3f* Vec3f_Up(Vec3f* dest, s16 pitch, s16 yaw, s16 roll);
f32 Math_DelSmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
f64 Math_DelSmoothStepToD(f64* pValue, f64 target, f64 fraction, f64 step, f64 minStep);
s16 Math_SmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep);
int Math_SmoothStepToI(int* pValue, int target, int scale, int step, int minStep);
void Rect_ToCRect(CRect* dst, Rect* src);
void Rect_ToRect(Rect* dst, CRect* src);
bool Rect_Check_PosIntersect(Rect* rect, Vec2s* pos);
Rect Rect_Translate(Rect r, s32 x, s32 y);
Rect Rect_New(s32 x, s32 y, s32 w, s32 h);
Rect Rect_Add(Rect a, Rect b);
Rect Rect_Sub(Rect a, Rect b);
Rect Rect_AddPos(Rect a, Rect b);
Rect Rect_SubPos(Rect a, Rect b);
bool Rect_PointIntersect(Rect* rect, s32 x, s32 y);
Vec2s Rect_ClosestPoint(Rect* rect, s32 x, s32 y);
Vec2s Rect_MidPoint(Rect* rect);
f32 Rect_PointDistance(Rect* rect, s32 x, s32 y);
int RectW(Rect r);
int RectH(Rect r);
int Rect_RectIntersect(Rect r, Rect i);
Rect Rect_Clamp(Rect r, Rect l);
Rect Rect_FlipHori(Rect r, Rect p);
Rect Rect_FlipVerti(Rect r, Rect p);
Rect Rect_ExpandX(Rect r, int amount);
Rect Rect_ShrinkX(Rect r, int amount);
Rect Rect_ExpandY(Rect r, int amount);
Rect Rect_ShrinkY(Rect r, int amount);
Rect Rect_Scale(Rect r, int x, int y);
Rect Rect_Vec2x2(Vec2s a, Vec2s b);
BoundBox BoundBox_New3F(Vec3f point);
BoundBox BoundBox_New2F(Vec2f point);
void BoundBox_Adjust3F(BoundBox* this, Vec3f point);
void BoundBox_Adjust2F(BoundBox* this, Vec2f point);

bool Vec2f_PointInShape(Vec2f p, Vec2f* poly, u32 numPoly);

Vec3f Vec3f_Cross(Vec3f a, Vec3f b);
Vec3s Vec3s_Cross(Vec3s a, Vec3s b);

f32 Vec3f_DistXZ(Vec3f a, Vec3f b);
f32 Vec3f_DistXYZ(Vec3f a, Vec3f b);
f32 Vec3s_DistXZ(Vec3s a, Vec3s b);
f32 Vec3s_DistXYZ(Vec3s a, Vec3s b);
f32 Vec2f_DistXZ(Vec2f a, Vec2f b);
f32 Vec2s_DistXZ(Vec2s a, Vec2s b);

s16 Vec3f_Yaw(Vec3f a, Vec3f b);
s16 Vec2f_Yaw(Vec2f a, Vec2f b);

s16 Vec3f_Pitch(Vec3f a, Vec3f b);

Vec2f Vec2f_Sub(Vec2f a, Vec2f b);
Vec3f Vec3f_Sub(Vec3f a, Vec3f b);
Vec4f Vec4f_Sub(Vec4f a, Vec4f b);

Vec2s Vec2s_Sub(Vec2s a, Vec2s b);
Vec3s Vec3s_Sub(Vec3s a, Vec3s b);
Vec4s Vec4s_Sub(Vec4s a, Vec4s b);

Vec2f Vec2f_Add(Vec2f a, Vec2f b);
Vec3f Vec3f_Add(Vec3f a, Vec3f b);
Vec4f Vec4f_Add(Vec4f a, Vec4f b);

Vec2s Vec2s_Add(Vec2s a, Vec2s b);
Vec3s Vec3s_Add(Vec3s a, Vec3s b);
Vec4s Vec4s_Add(Vec4s a, Vec4s b);

Vec2f Vec2f_Div(Vec2f a, Vec2f b);
Vec3f Vec3f_Div(Vec3f a, Vec3f b);
Vec4f Vec4f_Div(Vec4f a, Vec4f b);

Vec2f Vec2f_DivVal(Vec2f a, f32 val);
Vec3f Vec3f_DivVal(Vec3f a, f32 val);
Vec4f Vec4f_DivVal(Vec4f a, f32 val);

Vec2s Vec2s_Div(Vec2s a, Vec2s b);
Vec3s Vec3s_Div(Vec3s a, Vec3s b);
Vec4s Vec4s_Div(Vec4s a, Vec4s b);

Vec2s Vec2s_DivVal(Vec2s a, f32 val);
Vec3s Vec3s_DivVal(Vec3s a, f32 val);
Vec4s Vec4s_DivVal(Vec4s a, f32 val);

Vec2f Vec2f_Mul(Vec2f a, Vec2f b);
Vec3f Vec3f_Mul(Vec3f a, Vec3f b);
Vec4f Vec4f_Mul(Vec4f a, Vec4f b);

Vec2f Vec2f_MulVal(Vec2f a, f32 val);
Vec3f Vec3f_MulVal(Vec3f a, f32 val);
Vec4f Vec4f_MulVal(Vec4f a, f32 val);

Vec2s Vec2s_Mul(Vec2s a, Vec2s b);
Vec3s Vec3s_Mul(Vec3s a, Vec3s b);
Vec4s Vec4s_Mul(Vec4s a, Vec4s b);

Vec2s Vec2s_MulVal(Vec2s a, f32 val);
Vec3s Vec3s_MulVal(Vec3s a, f32 val);
Vec4s Vec4s_MulVal(Vec4s a, f32 val);

Vec2f Vec2f_New(f32 x, f32 y);
Vec3f Vec3f_New(f32 x, f32 y, f32 z);
Vec4f Vec4f_New(f32 x, f32 y, f32 z, f32 w);
Vec2s Vec2s_New(s16 x, s16 y);
Vec3s Vec3s_New(s16 x, s16 y, s16 z);
Vec4s Vec4s_New(s16 x, s16 y, s16 z, s16 w);

f32 Vec2f_Dot(Vec2f a, Vec2f b);
f32 Vec3f_Dot(Vec3f a, Vec3f b);
f32 Vec4f_Dot(Vec4f a, Vec4f b);
f32 Vec2s_Dot(Vec2s a, Vec2s b);
f32 Vec3s_Dot(Vec3s a, Vec3s b);
f32 Vec4s_Dot(Vec4s a, Vec4s b);

f32 Vec2f_MagnitudeSQ(Vec2f a);
f32 Vec3f_MagnitudeSQ(Vec3f a);
f32 Vec4f_MagnitudeSQ(Vec4f a);
f32 Vec2s_MagnitudeSQ(Vec2s a);
f32 Vec3s_MagnitudeSQ(Vec3s a);
f32 Vec4s_MagnitudeSQ(Vec4s a);

f32 Vec2f_Magnitude(Vec2f a);
f32 Vec3f_Magnitude(Vec3f a);
f32 Vec4f_Magnitude(Vec4f a);
f32 Vec2s_Magnitude(Vec2s a);
f32 Vec3s_Magnitude(Vec3s a);
f32 Vec4s_Magnitude(Vec4s a);

Vec2f Vec2f_Median(Vec2f a, Vec2f b);
Vec3f Vec3f_Median(Vec3f a, Vec3f b);
Vec4f Vec4f_Median(Vec4f a, Vec4f b);
Vec2s Vec2s_Median(Vec2s a, Vec2s b);
Vec3s Vec3s_Median(Vec3s a, Vec3s b);
Vec4s Vec4s_Median(Vec4s a, Vec4s b);

Vec2f Vec2f_Normalize(Vec2f a);
Vec3f Vec3f_Normalize(Vec3f a);
Vec4f Vec4f_Normalize(Vec4f a);
Vec2s Vec2s_Normalize(Vec2s a);
Vec3s Vec3s_Normalize(Vec3s a);
Vec4s Vec4s_Normalize(Vec4s a);

Vec2f Vec2f_LineSegDir(Vec2f a, Vec2f b);
Vec3f Vec3f_LineSegDir(Vec3f a, Vec3f b);
Vec4f Vec4f_LineSegDir(Vec4f a, Vec4f b);
Vec2s Vec2s_LineSegDir(Vec2s a, Vec2s b);
Vec3s Vec3s_LineSegDir(Vec3s a, Vec3s b);
Vec4s Vec4s_LineSegDir(Vec4s a, Vec4s b);

Vec2f Vec2f_Project(Vec2f a, Vec2f b);
Vec3f Vec3f_Project(Vec3f a, Vec3f b);
Vec4f Vec4f_Project(Vec4f a, Vec4f b);
Vec2s Vec2s_Project(Vec2s a, Vec2s b);
Vec3s Vec3s_Project(Vec3s a, Vec3s b);
Vec4s Vec4s_Project(Vec4s a, Vec4s b);

Vec2f Vec2f_Invert(Vec2f a);
Vec3f Vec3f_Invert(Vec3f a);
Vec4f Vec4f_Invert(Vec4f a);
Vec2s Vec2s_Invert(Vec2s a);
Vec3s Vec3s_Invert(Vec3s a);
Vec4s Vec4s_Invert(Vec4s a);

Vec2f Vec2f_InvMod(Vec2f a);
Vec3f Vec3f_InvMod(Vec3f a);
Vec4f Vec4f_InvMod(Vec4f a);

bool Vec2f_IsNaN(Vec2f a);
bool Vec3f_IsNaN(Vec3f a);
bool Vec4f_IsNaN(Vec4f a);

f32 Vec2f_Cos(Vec2f a, Vec2f b);
f32 Vec3f_Cos(Vec3f a, Vec3f b);
f32 Vec4f_Cos(Vec4f a, Vec4f b);
f32 Vec2s_Cos(Vec2s a, Vec2s b);
f32 Vec3s_Cos(Vec3s a, Vec3s b);
f32 Vec4s_Cos(Vec4s a, Vec4s b);

Vec2f Vec2f_Reflect(Vec2f vec, Vec2f normal);
Vec3f Vec3f_Reflect(Vec3f vec, Vec3f normal);
Vec4f Vec4f_Reflect(Vec4f vec, Vec4f normal);
Vec2s Vec2s_Reflect(Vec2s vec, Vec2s normal);
Vec3s Vec3s_Reflect(Vec3s vec, Vec3s normal);
Vec4s Vec4s_Reflect(Vec4s vec, Vec4s normal);

Vec3f Vec3f_ClosestPointOnRay(Vec3f rayStart, Vec3f rayEnd, Vec3f lineStart, Vec3f lineEnd);
Vec3f Vec3f_ProjectAlong(Vec3f point, Vec3f lineA, Vec3f lineB);

#endif
