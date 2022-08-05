#ifndef __VECTOR_H__
#define __VECTOR_H__
#include <ExtGui/Math.h>

#ifdef __IDE_FLAG__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifndef VEC_QF
#define VEC_QF static inline
#endif

#define UnfoldVec2(vec) (vec).x, (vec).y
#define UnfoldVec3(vec) (vec).x, (vec).y, (vec).z
#define UnfoldVec4(vec) (vec).x, (vec).y, (vec).z, (vec).w

VEC_QF Vec3f Math_Vec3f_Cross(Vec3f a, Vec3f b) {
	return (Vec3f) {
		       (a.y * b.z - b.y * a.z),
		       (a.z * b.x - b.z * a.x),
		       (a.x * b.y - b.x * a.y)
	};
}

VEC_QF Vec3s Math_Vec3s_Cross(Vec3s a, Vec3s b) {
	return (Vec3s) {
		       (a.y * b.z - b.y * a.z),
		       (a.z * b.x - b.z * a.x),
		       (a.x * b.y - b.x * a.y)
	};
}

VEC_QF f32 Math_Vec3f_DistXZ(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

VEC_QF f32 Math_Vec3f_DistXYZ(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dy = b.y - a.y;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

VEC_QF f32 Math_Vec3s_DistXZ(Vec3s a, Vec3s b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

VEC_QF f32 Math_Vec3s_DistXYZ(Vec3s a, Vec3s b) {
	f32 dx = b.x - a.x;
	f32 dy = b.y - a.y;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

VEC_QF f32 Math_Vec2f_DistXZ(Vec2f a, Vec2f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

VEC_QF f32 Math_Vec2s_DistXZ(Vec2s a, Vec2s b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

VEC_QF s16 Math_Vec3f_Yaw(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return Atan2S(dz, dx);
}

VEC_QF s16 Math_Vec2f_Yaw(Vec2f a, Vec2f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return Atan2S(dz, dx);
}

VEC_QF s16 Math_Vec3f_Pitch(Vec3f a, Vec3f b) {
	return Atan2S(Math_Vec3f_DistXZ(a, b), a.y - b.y);
}

VEC_QF Vec2f Math_Vec2f_Sub(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x - b.x, a.y - b.y };
}

VEC_QF Vec3f Math_Vec3f_Sub(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x - b.x, a.y - b.y, a.z - b.z };
}

VEC_QF Vec4f Math_Vec4f_Sub(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

VEC_QF Vec2f Math_Vec2f_SubVal(Vec2f a, f32 val) {
	return (Vec2f) { a.x - val, a.y - val };
}

VEC_QF Vec3f Math_Vec3f_SubVal(Vec3f a, f32 val) {
	return (Vec3f) { a.x - val, a.y - val, a.z - val };
}

VEC_QF Vec4f Math_Vec4f_SubVal(Vec4f a, f32 val) {
	return (Vec4f) { a.x - val, a.y - val, a.z - val, a.w - val };
}

VEC_QF Vec2s Math_Vec2s_Sub(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x - b.x, a.y - b.y };
}

VEC_QF Vec3s Math_Vec3s_Sub(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x - b.x, a.y - b.y, a.z - b.z };
}

VEC_QF Vec4s Math_Vec4s_Sub(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

VEC_QF Vec2s Math_Vec2s_SubVal(Vec2s a, s16 val) {
	return (Vec2s) { a.x - val, a.y - val };
}

VEC_QF Vec3s Math_Vec3s_SubVal(Vec3s a, s16 val) {
	return (Vec3s) { a.x - val, a.y - val, a.z - val };
}

VEC_QF Vec4s Math_Vec4s_SubVal(Vec4s a, s16 val) {
	return (Vec4s) { a.x - val, a.y - val, a.z - val, a.w - val };
}

VEC_QF Vec2f Math_Vec2f_Add(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x + b.x, a.y + b.y };
}

VEC_QF Vec3f Math_Vec3f_Add(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x + b.x, a.y + b.y, a.z + b.z };
}

VEC_QF Vec4f Math_Vec4f_Add(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

VEC_QF Vec2f Math_Vec2f_AddVal(Vec2f a, f32 val) {
	return (Vec2f) { a.x + val, a.y + val };
}

VEC_QF Vec3f Math_Vec3f_AddVal(Vec3f a, f32 val) {
	return (Vec3f) { a.x + val, a.y + val, a.z + val };
}

VEC_QF Vec4f Math_Vec4f_AddVal(Vec4f a, f32 val) {
	return (Vec4f) { a.x + val, a.y + val, a.z + val, a.w + val };
}

VEC_QF Vec2s Math_Vec2s_Add(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x + b.x, a.y + b.y };
}

VEC_QF Vec3s Math_Vec3s_Add(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x + b.x, a.y + b.y, a.z + b.z };
}

VEC_QF Vec4s Math_Vec4s_Add(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

VEC_QF Vec2s Math_Vec2s_AddVal(Vec2s a, s16 val) {
	return (Vec2s) { a.x + val, a.y + val };
}

VEC_QF Vec3s Math_Vec3s_AddVal(Vec3s a, s16 val) {
	return (Vec3s) { a.x + val, a.y + val, a.z + val };
}

VEC_QF Vec4s Math_Vec4s_AddVal(Vec4s a, s16 val) {
	return (Vec4s) { a.x + val, a.y + val, a.z + val, a.w + val };
}

VEC_QF Vec2f Math_Vec2f_Div(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x / b.x, a.y / b.y };
}

VEC_QF Vec3f Math_Vec3f_Div(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x / b.x, a.y / b.y, a.z / b.z };
}

VEC_QF Vec4f Math_Vec4f_Div(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

VEC_QF Vec2f Math_Vec2f_DivVal(Vec2f a, f32 val) {
	return (Vec2f) { a.x / val, a.y / val };
}

VEC_QF Vec3f Math_Vec3f_DivVal(Vec3f a, f32 val) {
	return (Vec3f) { a.x / val, a.y / val, a.z / val };
}

VEC_QF Vec4f Math_Vec4f_DivVal(Vec4f a, f32 val) {
	return (Vec4f) { a.x / val, a.y / val, a.z / val, a.w / val };
}

VEC_QF Vec2s Math_Vec2s_Div(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x / b.x, a.y / b.y };
}

VEC_QF Vec3s Math_Vec3s_Div(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x / b.x, a.y / b.y, a.z / b.z };
}

VEC_QF Vec4s Math_Vec4s_Div(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

VEC_QF Vec2s Math_Vec2s_DivVal(Vec2s a, f32 val) {
	return (Vec2s) { a.x / val, a.y / val };
}

VEC_QF Vec3s Math_Vec3s_DivVal(Vec3s a, f32 val) {
	return (Vec3s) { a.x / val, a.y / val, a.z / val };
}

VEC_QF Vec4s Math_Vec4s_DivVal(Vec4s a, f32 val) {
	return (Vec4s) { a.x / val, a.y / val, a.z / val, a.w / val };
}

VEC_QF Vec2f Math_Vec2f_Mul(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x* b.x, a.y* b.y };
}

VEC_QF Vec3f Math_Vec3f_Mul(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x* b.x, a.y* b.y, a.z* b.z };
}

VEC_QF Vec4f Math_Vec4f_Mul(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

VEC_QF Vec2f Math_Vec2f_MulVal(Vec2f a, f32 val) {
	return (Vec2f) { a.x* val, a.y* val };
}

VEC_QF Vec3f Math_Vec3f_MulVal(Vec3f a, f32 val) {
	return (Vec3f) { a.x* val, a.y* val, a.z* val };
}

VEC_QF Vec4f Math_Vec4f_MulVal(Vec4f a, f32 val) {
	return (Vec4f) { a.x* val, a.y* val, a.z* val, a.w* val };
}

VEC_QF Vec2s Math_Vec2s_Mul(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x* b.x, a.y* b.y };
}

VEC_QF Vec3s Math_Vec3s_Mul(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x* b.x, a.y* b.y, a.z* b.z };
}

VEC_QF Vec4s Math_Vec4s_Mul(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

VEC_QF Vec2s Math_Vec2s_MulVal(Vec2s a, f32 val) {
	return (Vec2s) { a.x* val, a.y* val };
}

VEC_QF Vec3s Math_Vec3s_MulVal(Vec3s a, f32 val) {
	return (Vec3s) { a.x* val, a.y* val, a.z* val };
}

VEC_QF Vec4s Math_Vec4s_MulVal(Vec4s a, f32 val) {
	return (Vec4s) { a.x* val, a.y* val, a.z* val, a.w* val };
}

VEC_QF Vec2f Math_Vec2f_New(f32 x, f32 y) {
	return (Vec2f) { x, y };
}

VEC_QF Vec3f Math_Vec3f_New(f32 x, f32 y, f32 z) {
	return (Vec3f) { x, y, z };
}

VEC_QF Vec4f Math_Vec4f_New(f32 x, f32 y, f32 z, f32 w) {
	return (Vec4f) { x, y, z, w };
}

VEC_QF Vec2s Math_Vec2s_New(s16 x, s16 y) {
	return (Vec2s) { x, y };
}

VEC_QF Vec3s Math_Vec3s_New(s16 x, s16 y, s16 z) {
	return (Vec3s) { x, y, z };
}

VEC_QF Vec4s Math_Vec4s_New(s16 x, s16 y, s16 z, s16 w) {
	return (Vec4s) { x, y, z, w };
}

VEC_QF f32 Math_Vec2f_Dot(Vec2f a, Vec2f b) {
	f32 dot = 0.0f;
	
	for (s32 i = 0; i < 2; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

VEC_QF f32 Math_Vec3f_Dot(Vec3f a, Vec3f b) {
	f32 dot = 0.0f;
	
	for (s32 i = 0; i < 3; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

VEC_QF f32 Math_Vec4f_Dot(Vec4f a, Vec4f b) {
	f32 dot = 0.0f;
	
	for (s32 i = 0; i < 4; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

VEC_QF f32 Math_Vec2s_Dot(Vec2s a, Vec2s b) {
	f32 dot = 0.0f;
	
	for (s32 i = 0; i < 2; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

VEC_QF f32 Math_Vec3s_Dot(Vec3s a, Vec3s b) {
	f32 dot = 0.0f;
	
	for (s32 i = 0; i < 3; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

VEC_QF f32 Math_Vec4s_Dot(Vec4s a, Vec4s b) {
	f32 dot = 0.0f;
	
	for (s32 i = 0; i < 4; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

VEC_QF f32 Math_Vec2f_MagnitudeSQ(Vec2f a) {
	return Math_Vec2f_Dot(a, a);
}

VEC_QF f32 Math_Vec3f_MagnitudeSQ(Vec3f a) {
	return Math_Vec3f_Dot(a, a);
}

VEC_QF f32 Math_Vec4f_MagnitudeSQ(Vec4f a) {
	return Math_Vec4f_Dot(a, a);
}

VEC_QF f32 Math_Vec2s_MagnitudeSQ(Vec2s a) {
	return Math_Vec2s_Dot(a, a);
}

VEC_QF f32 Math_Vec3s_MagnitudeSQ(Vec3s a) {
	return Math_Vec3s_Dot(a, a);
}

VEC_QF f32 Math_Vec4s_MagnitudeSQ(Vec4s a) {
	return Math_Vec4s_Dot(a, a);
}

VEC_QF f32 Math_Vec2f_Magnitude(Vec2f a) {
	return sqrtf(Math_Vec2f_MagnitudeSQ(a));
}

VEC_QF f32 Math_Vec3f_Magnitude(Vec3f a) {
	return sqrtf(Math_Vec3f_MagnitudeSQ(a));
}

VEC_QF f32 Math_Vec4f_Magnitude(Vec4f a) {
	return sqrtf(Math_Vec4f_MagnitudeSQ(a));
}

VEC_QF f32 Math_Vec2s_Magnitude(Vec2s a) {
	return sqrtf(Math_Vec2s_MagnitudeSQ(a));
}

VEC_QF f32 Math_Vec3s_Magnitude(Vec3s a) {
	return sqrtf(Math_Vec3s_MagnitudeSQ(a));
}

VEC_QF f32 Math_Vec4s_Magnitude(Vec4s a) {
	return sqrtf(Math_Vec4s_MagnitudeSQ(a));
}

VEC_QF Vec2f Math_Vec2f_Median(Vec2f a, Vec2f b) {
	Vec2f vec;
	
	for (s32 i = 0; i < 2; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

VEC_QF Vec3f Math_Vec3f_Median(Vec3f a, Vec3f b) {
	Vec3f vec;
	
	for (s32 i = 0; i < 3; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

VEC_QF Vec4f Math_Vec4f_Median(Vec4f a, Vec4f b) {
	Vec4f vec;
	
	for (s32 i = 0; i < 4; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

VEC_QF Vec2s Math_Vec2s_Median(Vec2s a, Vec2s b) {
	Vec2s vec;
	
	for (s32 i = 0; i < 2; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

VEC_QF Vec3s Math_Vec3s_Median(Vec3s a, Vec3s b) {
	Vec3s vec;
	
	for (s32 i = 0; i < 3; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

VEC_QF Vec4s Math_Vec4s_Median(Vec4s a, Vec4s b) {
	Vec4s vec;
	
	for (s32 i = 0; i < 4; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

VEC_QF Vec2f Math_Vec2f_Normalize(Vec2f a) {
	f32 mgn = Math_Vec2f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Math_Vec2f_MulVal(a, 0.0f);
	else
		return Math_Vec2f_DivVal(a, mgn);
}

VEC_QF Vec3f Math_Vec3f_Normalize(Vec3f a) {
	f32 mgn = Math_Vec3f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Math_Vec3f_MulVal(a, 0.0f);
	else
		return Math_Vec3f_DivVal(a, mgn);
}

VEC_QF Vec4f Math_Vec4f_Normalize(Vec4f a) {
	f32 mgn = Math_Vec4f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Math_Vec4f_MulVal(a, 0.0f);
	else
		return Math_Vec4f_DivVal(a, mgn);
}

VEC_QF Vec2s Math_Vec2s_Normalize(Vec2s a) {
	f32 mgn = Math_Vec2s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Math_Vec2s_MulVal(a, 0.0f);
	else
		return Math_Vec2s_DivVal(a, mgn);
}

VEC_QF Vec3s Math_Vec3s_Normalize(Vec3s a) {
	f32 mgn = Math_Vec3s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Math_Vec3s_MulVal(a, 0.0f);
	else
		return Math_Vec3s_DivVal(a, mgn);
}

VEC_QF Vec4s Math_Vec4s_Normalize(Vec4s a) {
	f32 mgn = Math_Vec4s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Math_Vec4s_MulVal(a, 0.0f);
	else
		return Math_Vec4s_DivVal(a, mgn);
}

VEC_QF Vec2f Math_Vec2f_LineSegDir(Vec2f a, Vec2f b) {
	return Math_Vec2f_Normalize(Math_Vec2f_Sub(b, a));
}

VEC_QF Vec3f Math_Vec3f_LineSegDir(Vec3f a, Vec3f b) {
	return Math_Vec3f_Normalize(Math_Vec3f_Sub(b, a));
}

VEC_QF Vec4f Math_Vec4f_LineSegDir(Vec4f a, Vec4f b) {
	return Math_Vec4f_Normalize(Math_Vec4f_Sub(b, a));
}

VEC_QF Vec2s Math_Vec2s_LineSegDir(Vec2s a, Vec2s b) {
	return Math_Vec2s_Normalize(Math_Vec2s_Sub(b, a));
}

VEC_QF Vec3s Math_Vec3s_LineSegDir(Vec3s a, Vec3s b) {
	return Math_Vec3s_Normalize(Math_Vec3s_Sub(b, a));
}

VEC_QF Vec4s Math_Vec4s_LineSegDir(Vec4s a, Vec4s b) {
	return Math_Vec4s_Normalize(Math_Vec4s_Sub(b, a));
}

#endif