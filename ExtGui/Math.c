#include "Math.h"

f64 gDeltaTime = 0;

s16 Atan2S(f32 x, f32 y) {
	return RadToBin(atan2f(y, x));
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

Rect Rect_New(s32 x, s32 y, s32 w, s32 h) {
	Rect dest = { x, y, w, h };
	
	return dest;
}

Rect Rect_Add(Rect a, Rect b) {
	return (Rect) {
		       a.x + b.x,
		       a.y + b.y,
		       a.w + b.w,
		       a.h + b.h,
	};
}

Rect Rect_Sub(Rect a, Rect b) {
	return (Rect) {
		       a.x - b.x,
		       a.y - b.y,
		       a.w - b.w,
		       a.h - b.h,
	};
}

Rect Rect_AddPos(Rect a, Rect b) {
	return (Rect) {
		       a.x + b.x,
		       a.y + b.y,
		       a.w,
		       a.h,
	};
}

Rect Rect_SubPos(Rect a, Rect b) {
	return (Rect) {
		       a.x - b.x,
		       a.y - b.y,
		       a.w,
		       a.h,
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

Vec2s Rect_MidPoint(Rect* rect) {
	return (Vec2s) {
		       rect->x + rect->w * 0.5,
		       rect->y + rect->h * 0.5
	};
}

f32 Rect_PointDistance(Rect* rect, s32 x, s32 y) {
	Vec2s p = { x, y };
	Vec2s r = Rect_ClosestPoint(rect, x, y);
	
	return Math_Vec2s_DistXZ(r, p);
}
