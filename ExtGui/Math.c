#include "Math.h"

f64 gDeltaTime = 0;

s16 Atan2S(f32 x, f32 y) {
	return RadToBin(atan2f(y, x));
}

f32 Math_Vec3f_DistXZ(Vec3f* a, Vec3f* b) {
	f32 dx = b->x - a->x;
	f32 dz = b->z - a->z;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Math_Vec3f_DistXYZ(Vec3f* a, Vec3f* b) {
	f32 dx = b->x - a->x;
	f32 dy = b->y - a->y;
	f32 dz = b->z - a->z;
	
	return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

f32 Math_Vec3s_DistXZ(Vec3s* a, Vec3s* b) {
	f32 dx = b->x - a->x;
	f32 dz = b->z - a->z;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Math_Vec3s_DistXYZ(Vec3s* a, Vec3s* b) {
	f32 dx = b->x - a->x;
	f32 dy = b->y - a->y;
	f32 dz = b->z - a->z;
	
	return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

f32 Math_Vec2f_DistXZ(Vec2f* a, Vec2f* b) {
	f32 dx = b->x - a->x;
	f32 dz = b->y - a->y;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Math_Vec2s_DistXZ(Vec2s* a, Vec2s* b) {
	f32 dx = b->x - a->x;
	f32 dz = b->y - a->y;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

s16 Math_Vec3f_Yaw(Vec3f* a, Vec3f* b) {
	f32 dx = b->x - a->x;
	f32 dz = b->z - a->z;
	
	return Atan2S(dz, dx);
}

s16 Math_Vec2f_Yaw(Vec2f* a, Vec2f* b) {
	f32 dx = b->x - a->x;
	f32 dz = b->y - a->y;
	
	return Atan2S(dz, dx);
}

s16 Math_Vec3f_Pitch(Vec3f* a, Vec3f* b) {
	return Atan2S(Math_Vec3f_DistXZ(a, b), a->y - b->y);
}

void Math_VecSphToVec3f(Vec3f* dest, VecSph* sph) {
	sph->pitch = Clamp(sph->pitch, DegToBin(-89.995f), DegToBin(89.995f));
	dest->x = sph->r * SinS(sph->pitch - DegToBin(90)) * SinS(sph->yaw);
	dest->y = sph->r * CosS(sph->pitch - DegToBin(90));
	dest->z = sph->r * SinS(sph->pitch - DegToBin(90)) * CosS(sph->yaw);
}

void Math_AddVecSphToVec3f(Vec3f* dest, VecSph* sph) {
	sph->pitch = Clamp(sph->pitch, DegToBin(-89.995f), DegToBin(89.995f));
	dest->x += sph->r * SinS(sph->pitch - DegToBin(90)) * SinS(sph->yaw);
	dest->y += sph->r * CosS(sph->pitch - DegToBin(90));
	dest->z += sph->r * SinS(sph->pitch - DegToBin(90)) * CosS(sph->yaw);
}

VecSph* Math_Vec3fToVecSph(VecSph* dest, Vec3f* vec) {
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

VecSph* Math_Vec3fToVecSphGeo(VecSph* dest, Vec3f* vec) {
	VecSph sph;
	
	Math_Vec3fToVecSph(&sph, vec);
	sph.pitch = 0x3FFF - sph.pitch;
	
	*dest = sph;
	
	return dest;
}

VecSph* Math_Vec3fDiffToVecSphGeo(VecSph* dest, Vec3f* a, Vec3f* b) {
	Vec3f sph;
	
	sph.x = b->x - a->x;
	sph.y = b->y - a->y;
	sph.z = b->z - a->z;
	
	return Math_Vec3fToVecSphGeo(dest, &sph);
}

Vec3f* Math_CalcUpFromPitchYawRoll(Vec3f* dest, s16 pitch, s16 yaw, s16 roll) {
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

f32 Math_DelSmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep) {
	step = step * gDeltaTime;
	minStep = minStep * gDeltaTime;
	
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

f64 Math_DelSmoothStepToD(f64* pValue, f64 target, f64 fraction, f64 step, f64 minStep) {
	step = step * gDeltaTime;
	minStep = minStep * gDeltaTime;
	
	if (*pValue != target) {
		f64 stepSize = (target - *pValue) * fraction;
		
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
	
	return fabs(target - *pValue);
}

s16 Math_DelSmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep) {
	step = step * gDeltaTime;
	minStep = minStep * gDeltaTime;
	s16 stepSize = 0;
	s16 diff = target - *pValue;
	
	if (*pValue != target) {
		stepSize = diff / scale;
		
		if ((stepSize > minStep) || (stepSize < -minStep)) {
			if (stepSize > step) {
				stepSize = step;
			}
			
			if (stepSize < -step) {
				stepSize = -step;
			}
			
			*pValue += stepSize;
		} else {
			if (diff >= 0) {
				*pValue += minStep;
				
				if ((s16)(target - *pValue) <= 0) {
					*pValue = target;
				}
			} else {
				*pValue -= minStep;
				
				if ((s16)(target - *pValue) >= 0) {
					*pValue = target;
				}
			}
		}
	}
	
	return diff;
}

void Rect_ToCRect(CRect* dst, Rect* src) {
	if (dst) {
		dst->x1 = src->x;
		dst->y1 = src->y;
		dst->x2 = src->x + src->w;
		dst->y2 = src->y + src->h;
	}
}

void Rect_ToRect(Rect* dst, CRect* src) {
	if (dst) {
		dst->x = src->x1;
		dst->y = src->y1;
		dst->w = src->x2 + src->x1;
		dst->h = src->y2 + src->y1;
	}
}

bool Rect_Check_PosIntersect(Rect* rect, Vec2s* pos) {
	return !(
		pos->y < rect->y ||
		pos->y > rect->h ||
		pos->x < rect->x ||
		pos->x > rect->w
	);
}

void Rect_Translate(Rect* rect, s32 x, s32 y) {
	rect->x += x;
	rect->w += x;
	rect->y += y;
	rect->h += y;
}

void Rect_Verify(Rect* rect) {
	if (rect->x > rect->w) {
		Swap(rect->x, rect->w);
	}
	if (rect->y > rect->h) {
		Swap(rect->y, rect->h);
	}
}

void Rect_Set(Rect* dest, s32 x, s32 w, s32 y, s32 h) {
	dest->x = x;
	dest->w = w;
	dest->y = y;
	dest->h = h;
	Rect_Verify(dest);
}

Rect Rect_Add(Rect* a, Rect* b) {
	return (Rect) {
		       a->x + b->x,
		       a->y + b->y,
		       a->w + b->w,
		       a->h + b->h,
	};
}

Rect Rect_Sub(Rect* a, Rect* b) {
	return (Rect) {
		       a->x - b->x,
		       a->y - b->y,
		       a->w - b->w,
		       a->h - b->h,
	};
}

Rect Rect_AddPos(Rect* a, Rect* b) {
	return (Rect) {
		       a->x + b->x,
		       a->y + b->y,
		       a->w,
		       a->h,
	};
}

Rect Rect_SubPos(Rect* a, Rect* b) {
	return (Rect) {
		       a->x - b->x,
		       a->y - b->y,
		       a->w,
		       a->h,
	};
}

bool Rect_PointIntersect(Rect* rect, s32 x, s32 y) {
	if (x >= rect->x && x < rect->x + rect->w)
		if (y >= rect->y && y < rect->y + rect->h)
			return true;
	
	return false;
}

Vec2s Rect_ClosestPoint(Rect* rect, s32 x, s32 y) {
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

f32 Rect_PointDistance(Rect* rect, s32 x, s32 y) {
	Vec2s p = { x, y };
	Vec2s r = Rect_ClosestPoint(rect, x, y);
	
	return Math_Vec2s_DistXZ(&r, &p);
}

#undef Vector_New
#define Vector_New(format, type) \
	Vec2 ## format Math_Vec2 ## format ## _New(type x, type y) { \
		return (Vec2 ## format) { x, y }; \
	} \
	Vec3 ## format Math_Vec3 ## format ## _New(type x, type y, type z) { \
		return (Vec3 ## format) { x, y, z }; \
	} \
	Vec4 ## format Math_Vec4 ## format ## _New(type x, type y, type z, type w) { \
		return (Vec4 ## format) { x, y, z, w }; \
	}

Vector_New(f, f32)
// Vector_New(i, s32)
Vector_New(s, s16)

#define Vector_Equal(format, type) \
	bool Math_Vec2 ## format ## _Equal(Vec2 ## format a, Vec2 ## format b) { \
		return !memcmp(&a, &b, sizeof(a)); \
	} \
	bool Math_Vec3 ## format ## _Equal(Vec3 ## format a, Vec3 ## format b) { \
		return !memcmp(&a, &b, sizeof(a)); \
	} \
	bool Math_Vec4 ## format ## _Equal(Vec4 ## format a, Vec4 ## format b) { \
		return !memcmp(&a, &b, sizeof(a)); \
	}

Vector_Equal(f, f32)
// Vector_Equal(i, s32)
Vector_Equal(s, s16)

#define Vector_Dot(format, type) \
	f32 Math_Vec2 ## format ## _Dot(Vec2 ## format a, Vec2 ## format b) { \
		f32 dot = 0.0f; \
		for (s32 i = 0; i < 2; i++) \
		dot += a.axis[i] *b.axis[i]; \
		return dot; \
	} \
	f32 Math_Vec3 ## format ## _Dot(Vec3 ## format a, Vec3 ## format b) { \
		f32 dot = 0.0f; \
		for (s32 i = 0; i < 3; i++) \
		dot += a.axis[i] *b.axis[i]; \
		return dot; \
	} \
	f32 Math_Vec4 ## format ## _Dot(Vec4 ## format a, Vec4 ## format b) { \
		f32 dot = 0.0f; \
		for (s32 i = 0; i < 4; i++) \
		dot += a.axis[i] *b.axis[i]; \
		return dot; \
	}

Vector_Dot(f, f32)
// Vector_Dot(i, s32)
Vector_Dot(s, s16)

#define Vector_MgnSQ(format, type) \
	f32 Math_Vec2 ## format ## _MagnitudeSQ(Vec2 ## format a) { \
		return Math_Vec2 ## format ## _Dot(a, a); \
	} \
	f32 Math_Vec3 ## format ## _MagnitudeSQ(Vec3 ## format a) { \
		return Math_Vec3 ## format ## _Dot(a, a); \
	} \
	f32 Math_Vec4 ## format ## _MagnitudeSQ(Vec4 ## format a) { \
		return Math_Vec4 ## format ## _Dot(a, a); \
	}

#define Vector_Mgn(format, type) \
	f32 Math_Vec2 ## format ## _Magnitude(Vec2 ## format a) { \
		return sqrtf(Math_Vec2 ## format ## _MagnitudeSQ(a)); \
	} \
	f32 Math_Vec3 ## format ## _Magnitude(Vec3 ## format a) { \
		return sqrtf(Math_Vec3 ## format ## _MagnitudeSQ(a)); \
	} \
	f32 Math_Vec4 ## format ## _Magnitude(Vec4 ## format a) { \
		return sqrtf(Math_Vec4 ## format ## _MagnitudeSQ(a)); \
	}

Vector_MgnSQ(f, f32)
// Vector_MgnSQ(i, s32)
Vector_MgnSQ(s, s16)
Vector_Mgn(f, f32)
// Vector_Mgn(i, s32)
Vector_Mgn(s, s16)

Vec3f Math_Vec3f_Cross(Vec3f a, Vec3f b) {
	return (Vec3f) {
		       (a.y * b.z - b.y * a.z),
		       (a.z * b.x - b.z * a.x),
		       (a.x * b.y - b.x * a.y)
	};
}
// Vec3i Math_Vec3i_Cross(Vec3i a, Vec3i b) {
// 	return (Vec3i) {
// 		       (a.y * b.z - b.y * a.z),
// 		       (a.z * b.x - b.z * a.x),
// 		       (a.x * b.y - b.x * a.y)
// 	};
// }
Vec3s Math_Vec3s_Cross(Vec3s a, Vec3s b) {
	return (Vec3s) {
		       (a.y * b.z - b.y * a.z),
		       (a.z * b.x - b.z * a.x),
		       (a.x * b.y - b.x * a.y)
	};
}

#define Vector_Nrm(format, type) \
	void Math_Vec2 ## format ## _Normalize(Vec2 ## format a) { \
		f32 mgn = Math_Vec2 ## format ## _Magnitude(a); \
		if (mgn == 0.0f) Math_Vec2 ## format ## _MulVal(a, 0.0f); \
		else Math_Vec2 ## format ## _DivVal(a, mgn); \
	} \
	void Math_Vec3 ## format ## _Normalize(Vec3 ## format a) { \
		f32 mgn = Math_Vec3 ## format ## _Magnitude(a); \
		if (mgn == 0.0f) Math_Vec3 ## format ## _MulVal(a, 0.0f); \
		else Math_Vec3 ## format ## _DivVal(a, mgn); \
	} \
	void Math_Vec4 ## format ## _Normalize(Vec4 ## format a) { \
		f32 mgn = Math_Vec4 ## format ## _Magnitude(a); \
		if (mgn == 0.0f) Math_Vec4 ## format ## _MulVal(a, 0.0f); \
		else Math_Vec4 ## format ## _DivVal(a, mgn); \
	}

Vector_Nrm(f, f32)
// Vector_Nrm(i, s32)
Vector_Nrm(s, s16)

#undef Vector_Math
#define Vector_Math(format, type, name, operator) \
	Vec2 ## format Math_Vec2 ## format ## _ ## name (Vec2 ## format a, Vec2 ## format b) { \
		Vec2 ## format vec = a; \
		for (s32 i = 0; i < 2; i++) \
		vec.axis[i] = a.axis[i] operator b.axis[i]; \
		return vec; \
	} \
	Vec3 ## format Math_Vec3 ## format ## _ ## name (Vec3 ## format a, Vec3 ## format b) { \
		Vec3 ## format vec = a; \
		for (s32 i = 0; i < 3; i++) \
		vec.axis[i] = a.axis[i] operator b.axis[i]; \
		return vec; \
	} \
	Vec4 ## format Math_Vec4 ## format ## _ ## name (Vec4 ## format a, Vec4 ## format b) { \
		Vec4 ## format vec = a; \
		for (s32 i = 0; i < 4; i++) \
		vec.axis[i] = a.axis[i] operator b.axis[i]; \
		return vec; \
	} \
	Vec2 ## format Math_Vec2 ## format ## _ ## name ## Val(Vec2 ## format a, type val) { \
		Vec2 ## format vec = a; \
		for (s32 i = 0; i < 2; i++) \
		vec.axis[i] = a.axis[i] operator val; \
		return vec; \
	} \
	Vec3 ## format Math_Vec3 ## format ## _ ## name ## Val(Vec3 ## format a, type val) { \
		Vec3 ## format vec = a; \
		for (s32 i = 0; i < 3; i++) \
		vec.axis[i] = a.axis[i] operator val; \
		return vec; \
	} \
	Vec4 ## format Math_Vec4 ## format ## _ ## name ## Val(Vec4 ## format a, type val) { \
		Vec4 ## format vec = a; \
		for (s32 i = 0; i < 4; i++) \
		vec.axis[i] = a.axis[i] operator val; \
		return vec; \
	}

Vector_Math(f, f32, Sub, -)
// Vector_Math(i, s32, Sub, -)
Vector_Math(s, s16, Sub, -)

Vector_Math(f, f32, Add, +)
// Vector_Math(i, s32, Add, +)
Vector_Math(s, s16, Add, +)

Vector_Math(f, f32, Div, /)
// Vector_Math(i, s32, Div, /)
Vector_Math(s, s16, Div, /)

Vector_Math(f, f32, Mul, *)
// Vector_Math(i, s32, Mul, *)
Vector_Math(s, s16, Mul, *)