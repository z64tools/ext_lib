#ifndef __VECTOR_H__
#define __VECTOR_H__
#include <ExtGui/Math.h>

#ifdef __IDE_FLAG__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifndef VEC_QF
#define VEC_QF static inline
#endif

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

#undef Vector_Math
#define Vector_Math(format, type, name, operator) \
	VEC_QF Vec2 ## format Math_Vec2 ## format ## _ ## name (Vec2 ## format a, Vec2 ## format b) { \
		return (Vec2 ## format) { a.x operator b.x, a.y operator b.y }; \
	} \
	VEC_QF Vec3 ## format Math_Vec3 ## format ## _ ## name (Vec3 ## format a, Vec3 ## format b) { \
		return (Vec3 ## format) { a.x operator b.x, a.y operator b.y, a.z operator b.z }; \
	} \
	VEC_QF Vec4 ## format Math_Vec4 ## format ## _ ## name (Vec4 ## format a, Vec4 ## format b) { \
		return (Vec4 ## format) { a.x operator b.x, a.y operator b.y, a.z operator b.z, a.w operator b.w }; \
	} \
	VEC_QF Vec2 ## format Math_Vec2 ## format ## _ ## name ## Val(Vec2 ## format a, type val) { \
		return (Vec2 ## format) { a.x operator val, a.y operator val }; \
	} \
	VEC_QF Vec3 ## format Math_Vec3 ## format ## _ ## name ## Val(Vec3 ## format a, type val) { \
		return (Vec3 ## format) { a.x operator val, a.y operator val, a.z operator val }; \
	} \
	VEC_QF Vec4 ## format Math_Vec4 ## format ## _ ## name ## Val(Vec4 ## format a, type val) { \
		return (Vec4 ## format) { a.x operator val, a.y operator val, a.z operator val, a.w operator val }; \
	}

Vector_Math(f, f32, Sub, -)
Vector_Math(s, s16, Sub, -)

Vector_Math(f, f32, Add, +)
Vector_Math(s, s16, Add, +)

Vector_Math(f, f32, Div, /)
Vector_Math(s, s16, Div, /)

Vector_Math(f, f32, Mul, *)
Vector_Math(s, s16, Mul, *)

#undef Vector_New
#define Vector_New(format, type) \
	VEC_QF Vec2 ## format Math_Vec2 ## format ## _New(type x, type y) { \
		return (Vec2 ## format) { x, y }; \
	} \
	VEC_QF Vec3 ## format Math_Vec3 ## format ## _New(type x, type y, type z) { \
		return (Vec3 ## format) { x, y, z }; \
	} \
	VEC_QF Vec4 ## format Math_Vec4 ## format ## _New(type x, type y, type z, type w) { \
		return (Vec4 ## format) { x, y, z, w }; \
	}

Vector_New(f, f32)
Vector_New(s, s16)

#define Vector_Dot(format, type) \
	VEC_QF f32 Math_Vec2 ## format ## _Dot(Vec2 ## format a, Vec2 ## format b) { \
		f32 dot = 0.0f; \
		for (s32 i = 0; i < 2; i++) \
		dot += a.axis[i] *b.axis[i]; \
		return dot; \
	} \
	VEC_QF f32 Math_Vec3 ## format ## _Dot(Vec3 ## format a, Vec3 ## format b) { \
		f32 dot = 0.0f; \
		for (s32 i = 0; i < 3; i++) \
		dot += a.axis[i] *b.axis[i]; \
		return dot; \
	} \
	VEC_QF f32 Math_Vec4 ## format ## _Dot(Vec4 ## format a, Vec4 ## format b) { \
		f32 dot = 0.0f; \
		for (s32 i = 0; i < 4; i++) \
		dot += a.axis[i] *b.axis[i]; \
		return dot; \
	}

Vector_Dot(f, f32)
Vector_Dot(s, s16)

#define Vector_MgnSQ(format, type) \
	VEC_QF f32 Math_Vec2 ## format ## _MagnitudeSQ(Vec2 ## format a) { \
		return Math_Vec2 ## format ## _Dot(a, a); \
	} \
	VEC_QF f32 Math_Vec3 ## format ## _MagnitudeSQ(Vec3 ## format a) { \
		return Math_Vec3 ## format ## _Dot(a, a); \
	} \
	VEC_QF f32 Math_Vec4 ## format ## _MagnitudeSQ(Vec4 ## format a) { \
		return Math_Vec4 ## format ## _Dot(a, a); \
	}

#define Vector_Mgn(format, type) \
	VEC_QF f32 Math_Vec2 ## format ## _Magnitude(Vec2 ## format a) { \
		return sqrtf(Math_Vec2 ## format ## _MagnitudeSQ(a)); \
	} \
	VEC_QF f32 Math_Vec3 ## format ## _Magnitude(Vec3 ## format a) { \
		return sqrtf(Math_Vec3 ## format ## _MagnitudeSQ(a)); \
	} \
	VEC_QF f32 Math_Vec4 ## format ## _Magnitude(Vec4 ## format a) { \
		return sqrtf(Math_Vec4 ## format ## _MagnitudeSQ(a)); \
	}

Vector_MgnSQ(f, f32)
Vector_MgnSQ(s, s16)
Vector_Mgn(f, f32)
Vector_Mgn(s, s16)

#define Vector_Median(format, type) \
	VEC_QF Vec2 ## format Math_Vec2 ## format ## _Median(Vec2 ## format a, Vec2 ## format b) { \
		Vec2 ## format vec; \
		for (s32 i = 0; i < 2; i++) vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f; \
		return vec; \
	} \
	VEC_QF Vec3 ## format Math_Vec3 ## format ## _Median(Vec3 ## format a, Vec3 ## format b) { \
		Vec3 ## format vec; \
		for (s32 i = 0; i < 3; i++) vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f; \
		return vec; \
	} \
	VEC_QF Vec4 ## format Math_Vec4 ## format ## _Median(Vec4 ## format a, Vec4 ## format b) { \
		Vec4 ## format vec; \
		for (s32 i = 0; i < 4; i++) vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f; \
		return vec; \
	}

Vector_Median(f, f32)
Vector_Median(s, s16)

#define Vector_Nrm(format, type) \
	VEC_QF Vec2 ## format Math_Vec2 ## format ## _Normalize(Vec2 ## format a) { \
		f32 mgn = Math_Vec2 ## format ## _Magnitude(a); \
		if (mgn == 0.0f) return Math_Vec2 ## format ## _MulVal(a, 0.0f); \
		else return Math_Vec2 ## format ## _DivVal(a, mgn); \
	} \
	VEC_QF Vec3 ## format Math_Vec3 ## format ## _Normalize(Vec3 ## format a) { \
		f32 mgn = Math_Vec3 ## format ## _Magnitude(a); \
		if (mgn == 0.0f) return Math_Vec3 ## format ## _MulVal(a, 0.0f); \
		else return Math_Vec3 ## format ## _DivVal(a, mgn); \
	} \
	VEC_QF Vec4 ## format Math_Vec4 ## format ## _Normalize(Vec4 ## format a) { \
		f32 mgn = Math_Vec4 ## format ## _Magnitude(a); \
		if (mgn == 0.0f) return Math_Vec4 ## format ## _MulVal(a, 0.0f); \
		else return Math_Vec4 ## format ## _DivVal(a, mgn); \
	}

Vector_Nrm(f, f32)
Vector_Nrm(s, s16)

#endif