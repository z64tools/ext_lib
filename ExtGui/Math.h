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
__EXT_VECTYPEDEF(s32, i)
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

#define Vec2_Substract(type, a, b) ( \
		(Vec2 ## type) { \
		a.x - b.x, \
		a.y - b.y, \
	})
#define Vec3_Substract(type, a, b) ( \
		(Vec3 ## type) { \
		a.x - b.x, \
		a.y - b.y, \
		a.z - b.z, \
	})
#define Vec4_Substract(type, a, b) ( \
		(Vec4 ## type) { \
		a.x - b.x, \
		a.y - b.y, \
		a.z - b.z, \
		a.w - b.w, \
	})

#define Vec2_Add(type, a, b) ( \
		(Vec2 ## type) { \
		a.x + b.x, \
		a.y + b.y, \
	})
#define Vec3_Add(type, a, b) ( \
		(Vec3 ## type) { \
		a.x + b.x, \
		a.y + b.y, \
		a.z + b.z, \
	})
#define Vec4_Add(type, a, b) ( \
		(Vec4 ## type) { \
		a.x + b.x, \
		a.y + b.y, \
		a.z + b.z, \
		a.w + b.w, \
	})

#define Vec2_Equal(a, b) ( \
		a.x == b.x && \
		a.y == b.y \
)
#define Vec3_Equal(a, b) ( \
		a.x == b.x && \
		a.y == b.y && \
		a.z == b.z \
)
#define Vec4_Equal(a, b) ( \
		a.x == b.x && \
		a.y == b.y && \
		a.z == b.z && \
		a.w == b.w \
)

#define Vec2_Copy(dest, src) { \
		dest.x = src.x; \
		dest.y = src.y; \
		dest.z = src.z; \
}
#define Vec3_Copy(dest, src) { \
		dest.x = src.x; \
		dest.y = src.y; \
		dest.z = src.z; \
}
#define Vec4_Copy(dest, src) { \
		dest.x = src.x; \
		dest.y = src.y; \
		dest.z = src.z; \
		dest.w = src.w; \
}

#define Vec2_MultVec(dest, src) { \
		dest.x *= src.x; \
		dest.y *= src.y; \
}
#define Vec3_MultVec(dest, src) { \
		dest.x *= src.x; \
		dest.y *= src.y; \
		dest.z *= src.z; \
}
#define Vec4_MultVec(dest, src) { \
		dest.x *= src.x; \
		dest.y *= src.y; \
		dest.z *= src.z; \
		dest.w *= src.w; \
}

#define Vec2_Mult(dest, src) { \
		dest.x *= src; \
		dest.y *= src; \
}
#define Vec3_Mult(dest, src) { \
		dest.x *= src; \
		dest.y *= src; \
		dest.z *= src; \
}
#define Vec4_Mult(dest, src) { \
		dest.x *= src; \
		dest.y *= src; \
		dest.z *= src; \
		dest.w *= src; \
}

#define Vec2_Dot(a, b) ({ \
		(a.x * b.x) + \
		(a.y * b.y); \
	})
#define Vec3_Dot(a, b) ({ \
		(a.x * b.x) + \
		(a.y * b.y) + \
		(a.z * b.z); \
	})
#define Vec4_Dot(a, b) ({ \
		(a.x * b.x) + \
		(a.y * b.y) + \
		(a.z * b.z) + \
		(a.w * b.w); \
	})

#define Vec3_Cross(a, b) ({ \
		(typeof(a)) { \
			a.y* b.z - b.y* a.z, \
			a.z* b.x - b.z* a.x, \
			a.x* b.y - b.x* a.y \
		}; \
	})

#define Vec2_Magnitude(a) ({ \
		sqrtf( \
			(a.x * a.x) + \
			(a.y * a.y) \
		); \
	})
#define Vec3_Magnitude(a) ({ \
		sqrtf( \
			(a.x * a.x) + \
			(a.y * a.y) + \
			(a.z * a.z) \
		); \
	})
#define Vec4_Magnitude(a) ({ \
		sqrtf( \
			(a.x * a.x) + \
			(a.y * a.y) + \
			(a.z * a.z) + \
			(a.w * a.w) \
		); \
	})

#define Vec2_Normalize(a) ({ \
		typeof(a) ret; \
		f64 mgn = Vec2_Magnitude(a); \
		if (mgn == 0) { \
			ret.x = ret.y = 0; \
		} else { \
			ret.x = a.x / mgn; \
			ret.y = a.y / mgn; \
		} \
		ret; \
	})
#define Vec3_Normalize(a) ({ \
		typeof(a) ret; \
		f64 mgn = Vec3_Magnitude(a); \
		if (mgn == 0) { \
			ret.x = ret.y = ret.z = 0; \
		} else { \
			ret.x = a.x / mgn; \
			ret.y = a.y / mgn; \
			ret.z = a.z / mgn; \
		} \
		ret; \
	})
#define Vec4_Normalize(a) ({ \
		typeof(a) ret; \
		f64 mgn = Vec4_Magnitude(a); \
		if (mgn == 0) { \
			ret.x = ret.y = ret.z = ret.w = 0; \
		} else { \
			ret.x = a.x / mgn; \
			ret.y = a.y / mgn; \
			ret.z = a.z / mgn; \
			ret.w = a.w / mgn; \
		} \
		ret; \
	})

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

#endif
