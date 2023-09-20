#include <ext_vector.h>
#include <string.h>

s16 Atan2S(f32 x, f32 y) {
	return RadToBin(atan2f(y, x));
}

void VecSphToVec3f(Vec3f* dest, VecSph* sph) {
	sph->pitch = clamp(sph->pitch, DegToBin(-89.995f), DegToBin(89.995f));
	dest->x = sph->r * SinS(sph->pitch - DegToBin(90)) * SinS(sph->yaw);
	dest->y = sph->r * CosS(sph->pitch - DegToBin(90));
	dest->z = sph->r * SinS(sph->pitch - DegToBin(90)) * CosS(sph->yaw);
}

void Math_AddVecSphToVec3f(Vec3f* dest, VecSph* sph) {
	sph->pitch = clamp(sph->pitch, DegToBin(-89.995f), DegToBin(89.995f));
	dest->x += sph->r * SinS(sph->pitch - DegToBin(90)) * SinS(sph->yaw);
	dest->y += sph->r * CosS(sph->pitch - DegToBin(90));
	dest->z += sph->r * SinS(sph->pitch - DegToBin(90)) * CosS(sph->yaw);
}

VecSph* VecSph_FromVec3f(VecSph* dest, Vec3f* vec) {
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

VecSph* VecSph_GeoFromVec3f(VecSph* dest, Vec3f* vec) {
	VecSph sph;
	
	VecSph_FromVec3f(&sph, vec);
	sph.pitch = 0x3FFF - sph.pitch;
	
	*dest = sph;
	
	return dest;
}

VecSph* VecSph_GeoFromVec3fDiff(VecSph* dest, Vec3f* a, Vec3f* b) {
	Vec3f sph;
	
	sph.x = b->x - a->x;
	sph.y = b->y - a->y;
	sph.z = b->z - a->z;
	
	return VecSph_GeoFromVec3f(dest, &sph);
}

Vec3f* Vec3f_Up(Vec3f* dest, s16 pitch, s16 yaw, s16 roll) {
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

f64 Math_DelSmoothStepToD(f64* pValue, f64 target, f64 fraction, f64 step, f64 minStep) {
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

s16 Math_SmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep) {
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

int Math_SmoothStepToI(int* pValue, int target, int scale, int step, int minStep) {
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

Rect Rect_Translate(Rect r, s32 x, s32 y) {
	r.x += x;
	r.w += x;
	r.y += y;
	r.h += y;
	
	return r;
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
	
	return Vec2s_DistXZ(r, p);
}

int RectW(Rect r) {
	return r.x + r.w;
}

int RectH(Rect r) {
	return r.y + r.h;
}

int Rect_RectIntersect(Rect r, Rect i) {
	if (r.x >= i.x + i.w || i.x >= r.x + r.w)
		return false;
	
	if (r.y >= i.y + i.h || i.y >= r.y + r.h)
		return false;
	
	return true;
}

Rect Rect_Clamp(Rect r, Rect l) {
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

Rect Rect_FlipHori(Rect r, Rect p) {
	r = Rect_SubPos(r, p);
	r.x = remapf(r.x, 0, p.w, p.w, 0) - r.w;
	return Rect_AddPos(r, p);
}

Rect Rect_FlipVerti(Rect r, Rect p) {
	r = Rect_SubPos(r, p);
	r.y = remapf(r.y, 0, p.h, p.h, 0) - r.h;
	return Rect_AddPos(r, p);
}

Rect Rect_ExpandX(Rect r, int amount) {
	if (amount < 0) {
		r.x -= abs(amount);
		r.w += abs(amount);
	} else {
		r.w += abs(amount);
	}
	
	return r;
}

Rect Rect_ShrinkX(Rect r, int amount) {
	if (amount < 0) {
		r.w -= abs(amount);
	} else {
		r.x += abs(amount);
		r.w -= abs(amount);
	}
	
	return r;
}

Rect Rect_ExpandY(Rect r, int amount) {
	amount = -amount;
	
	if (amount < 0) {
		r.y -= abs(amount);
		r.h += abs(amount);
	} else {
		r.h += abs(amount);
	}
	
	return r;
}

Rect Rect_ShrinkY(Rect r, int amount) {
	amount = -amount;
	
	if (amount < 0) {
		r.h -= abs(amount);
	} else {
		r.y += abs(amount);
		r.h -= abs(amount);
	}
	
	return r;
}

Rect Rect_Scale(Rect r, int x, int y) {
	x = -x;
	y = -y;
	
	r.x += (x);
	r.w -= (x) * 2;
	r.y += (y);
	r.h -= (y) * 2;
	
	return r;
}

Rect Rect_Vec2x2(Vec2s a, Vec2s b) {
	int minx = Min(a.x, b.x);
	int maxx = Max(a.x, b.x);
	int miny = Min(a.y, b.y);
	int maxy = Max(a.y, b.y);
	
	return Rect_New(minx, miny, maxx - minx, maxy - miny);
}

BoundBox BoundBox_New3F(Vec3f point) {
	BoundBox this;
	
	this.xMax = point.x;
	this.xMin = point.x;
	this.yMax = point.y;
	this.yMin = point.y;
	this.zMax = point.z;
	this.zMin = point.z;
	
	return this;
}

BoundBox BoundBox_New2F(Vec2f point) {
	BoundBox this = {};
	
	this.xMax = point.x;
	this.xMin = point.x;
	this.yMax = point.y;
	this.yMin = point.y;
	
	return this;
}

void BoundBox_Adjust3F(BoundBox* this, Vec3f point) {
	this->xMax = Max(this->xMax, point.x);
	this->xMin = Min(this->xMin, point.x);
	this->yMax = Max(this->yMax, point.y);
	this->yMin = Min(this->yMin, point.y);
	this->zMax = Max(this->zMax, point.z);
	this->zMin = Min(this->zMin, point.z);
}

void BoundBox_Adjust2F(BoundBox* this, Vec2f point) {
	this->xMax = Max(this->xMax, point.x);
	this->xMin = Min(this->xMin, point.x);
	this->yMax = Max(this->yMax, point.y);
	this->yMin = Min(this->yMin, point.y);
}

bool Vec2f_PointInShape(Vec2f p, Vec2f* poly, u32 numPoly) {
	bool in = false;
	
	for (u32 i = 0, j = numPoly - 1; i < numPoly; j = i++) {
		if ((poly[i].y > p.y) != (poly[j].y > p.y)
			&& p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x ) {
			in = !in;
		}
		
	}
	
	return in;
}

Vec3f Vec3f_Cross(Vec3f a, Vec3f b) {
	return (Vec3f) {
			   (a.y * b.z - b.y * a.z),
			   (a.z * b.x - b.z * a.x),
			   (a.x * b.y - b.x * a.y)
	};
}

Vec3s Vec3s_Cross(Vec3s a, Vec3s b) {
	return (Vec3s) {
			   (a.y * b.z - b.y * a.z),
			   (a.z * b.x - b.z * a.x),
			   (a.x * b.y - b.x * a.y)
	};
}

f32 Vec3f_DistXZ(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Vec3f_DistXYZ(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dy = b.y - a.y;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

f32 Vec3s_DistXZ(Vec3s a, Vec3s b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Vec3s_DistXYZ(Vec3s a, Vec3s b) {
	f32 dx = b.x - a.x;
	f32 dy = b.y - a.y;
	f32 dz = b.z - a.z;
	
	return sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
}

f32 Vec2f_DistXZ(Vec2f a, Vec2f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

f32 Vec2s_DistXZ(Vec2s a, Vec2s b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return sqrtf(SQ(dx) + SQ(dz));
}

s16 Vec3f_Yaw(Vec3f a, Vec3f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.z - a.z;
	
	return Atan2S(dz, dx);
}

s16 Vec2f_Yaw(Vec2f a, Vec2f b) {
	f32 dx = b.x - a.x;
	f32 dz = b.y - a.y;
	
	return Atan2S(dz, dx);
}

s16 Vec3f_Pitch(Vec3f a, Vec3f b) {
	return Atan2S(Vec3f_DistXZ(a, b), a.y - b.y);
}

Vec2f Vec2f_Sub(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x - b.x, a.y - b.y };
}

Vec3f Vec3f_Sub(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec4f Vec4f_Sub(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

Vec2s Vec2s_Sub(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x - b.x, a.y - b.y };
}

Vec3s Vec3s_Sub(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec4s Vec4s_Sub(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

Vec2f Vec2f_Add(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x + b.x, a.y + b.y };
}

Vec3f Vec3f_Add(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec4f Vec4f_Add(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

Vec2s Vec2s_Add(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x + b.x, a.y + b.y };
}

Vec3s Vec3s_Add(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec4s Vec4s_Add(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

Vec2f Vec2f_Div(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x / b.x, a.y / b.y };
}

Vec3f Vec3f_Div(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x / b.x, a.y / b.y, a.z / b.z };
}

Vec4f Vec4f_Div(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

Vec2f Vec2f_DivVal(Vec2f a, f32 val) {
	return (Vec2f) { a.x / val, a.y / val };
}

Vec3f Vec3f_DivVal(Vec3f a, f32 val) {
	return (Vec3f) { a.x / val, a.y / val, a.z / val };
}

Vec4f Vec4f_DivVal(Vec4f a, f32 val) {
	return (Vec4f) { a.x / val, a.y / val, a.z / val, a.w / val };
}

Vec2s Vec2s_Div(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x / b.x, a.y / b.y };
}

Vec3s Vec3s_Div(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x / b.x, a.y / b.y, a.z / b.z };
}

Vec4s Vec4s_Div(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

Vec2s Vec2s_DivVal(Vec2s a, f32 val) {
	return (Vec2s) { a.x / val, a.y / val };
}

Vec3s Vec3s_DivVal(Vec3s a, f32 val) {
	return (Vec3s) { a.x / val, a.y / val, a.z / val };
}

Vec4s Vec4s_DivVal(Vec4s a, f32 val) {
	return (Vec4s) { a.x / val, a.y / val, a.z / val, a.w / val };
}

Vec2f Vec2f_Mul(Vec2f a, Vec2f b) {
	return (Vec2f) { a.x* b.x, a.y* b.y };
}

Vec3f Vec3f_Mul(Vec3f a, Vec3f b) {
	return (Vec3f) { a.x* b.x, a.y* b.y, a.z* b.z };
}

Vec4f Vec4f_Mul(Vec4f a, Vec4f b) {
	return (Vec4f) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

Vec2f Vec2f_MulVal(Vec2f a, f32 val) {
	return (Vec2f) { a.x* val, a.y* val };
}

Vec3f Vec3f_MulVal(Vec3f a, f32 val) {
	return (Vec3f) { a.x* val, a.y* val, a.z* val };
}

Vec4f Vec4f_MulVal(Vec4f a, f32 val) {
	return (Vec4f) { a.x* val, a.y* val, a.z* val, a.w* val };
}

Vec2s Vec2s_Mul(Vec2s a, Vec2s b) {
	return (Vec2s) { a.x* b.x, a.y* b.y };
}

Vec3s Vec3s_Mul(Vec3s a, Vec3s b) {
	return (Vec3s) { a.x* b.x, a.y* b.y, a.z* b.z };
}

Vec4s Vec4s_Mul(Vec4s a, Vec4s b) {
	return (Vec4s) { a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w };
}

Vec2s Vec2s_MulVal(Vec2s a, f32 val) {
	return (Vec2s) { a.x* val, a.y* val };
}

Vec3s Vec3s_MulVal(Vec3s a, f32 val) {
	return (Vec3s) { a.x* val, a.y* val, a.z* val };
}

Vec4s Vec4s_MulVal(Vec4s a, f32 val) {
	return (Vec4s) { a.x* val, a.y* val, a.z* val, a.w* val };
}

Vec2f Vec2f_New(f32 x, f32 y) {
	return (Vec2f) { x, y };
}

Vec3f Vec3f_New(f32 x, f32 y, f32 z) {
	return (Vec3f) { x, y, z };
}

Vec4f Vec4f_New(f32 x, f32 y, f32 z, f32 w) {
	return (Vec4f) { x, y, z, w };
}

Vec2s Vec2s_New(s16 x, s16 y) {
	return (Vec2s) { x, y };
}

Vec3s Vec3s_New(s16 x, s16 y, s16 z) {
	return (Vec3s) { x, y, z };
}

Vec4s Vec4s_New(s16 x, s16 y, s16 z, s16 w) {
	return (Vec4s) { x, y, z, w };
}

f32 Vec2f_Dot(Vec2f a, Vec2f b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 2; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec3f_Dot(Vec3f a, Vec3f b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 3; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec4f_Dot(Vec4f a, Vec4f b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 4; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec2s_Dot(Vec2s a, Vec2s b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 2; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec3s_Dot(Vec3s a, Vec3s b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 3; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec4s_Dot(Vec4s a, Vec4s b) {
	f32 dot = 0.0f;
	
	for (int i = 0; i < 4; i++)
		dot += a.axis[i] * b.axis[i];
	
	return dot;
}

f32 Vec2f_MagnitudeSQ(Vec2f a) {
	return Vec2f_Dot(a, a);
}

f32 Vec3f_MagnitudeSQ(Vec3f a) {
	return Vec3f_Dot(a, a);
}

f32 Vec4f_MagnitudeSQ(Vec4f a) {
	return Vec4f_Dot(a, a);
}

f32 Vec2s_MagnitudeSQ(Vec2s a) {
	return Vec2s_Dot(a, a);
}

f32 Vec3s_MagnitudeSQ(Vec3s a) {
	return Vec3s_Dot(a, a);
}

f32 Vec4s_MagnitudeSQ(Vec4s a) {
	return Vec4s_Dot(a, a);
}

f32 Vec2f_Magnitude(Vec2f a) {
	return sqrtf(Vec2f_MagnitudeSQ(a));
}

f32 Vec3f_Magnitude(Vec3f a) {
	return sqrtf(Vec3f_MagnitudeSQ(a));
}

f32 Vec4f_Magnitude(Vec4f a) {
	return sqrtf(Vec4f_MagnitudeSQ(a));
}

f32 Vec2s_Magnitude(Vec2s a) {
	return sqrtf(Vec2s_MagnitudeSQ(a));
}

f32 Vec3s_Magnitude(Vec3s a) {
	return sqrtf(Vec3s_MagnitudeSQ(a));
}

f32 Vec4s_Magnitude(Vec4s a) {
	return sqrtf(Vec4s_MagnitudeSQ(a));
}

Vec2f Vec2f_Median(Vec2f a, Vec2f b) {
	Vec2f vec;
	
	for (int i = 0; i < 2; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec3f Vec3f_Median(Vec3f a, Vec3f b) {
	Vec3f vec;
	
	for (int i = 0; i < 3; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec4f Vec4f_Median(Vec4f a, Vec4f b) {
	Vec4f vec;
	
	for (int i = 0; i < 4; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec2s Vec2s_Median(Vec2s a, Vec2s b) {
	Vec2s vec;
	
	for (int i = 0; i < 2; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec3s Vec3s_Median(Vec3s a, Vec3s b) {
	Vec3s vec;
	
	for (int i = 0; i < 3; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec4s Vec4s_Median(Vec4s a, Vec4s b) {
	Vec4s vec;
	
	for (int i = 0; i < 4; i++)
		vec.axis[i] = (a.axis[i] + b.axis[i]) * 0.5f;
	
	return vec;
}

Vec2f Vec2f_Normalize(Vec2f a) {
	f32 mgn = Vec2f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec2f_MulVal(a, 0.0f);
	else
		return Vec2f_DivVal(a, mgn);
}

Vec3f Vec3f_Normalize(Vec3f a) {
	f32 mgn = Vec3f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec3f_MulVal(a, 0.0f);
	else
		return Vec3f_DivVal(a, mgn);
}

Vec4f Vec4f_Normalize(Vec4f a) {
	f32 mgn = Vec4f_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec4f_MulVal(a, 0.0f);
	else
		return Vec4f_DivVal(a, mgn);
}

Vec2s Vec2s_Normalize(Vec2s a) {
	f32 mgn = Vec2s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec2s_MulVal(a, 0.0f);
	else
		return Vec2s_DivVal(a, mgn);
}

Vec3s Vec3s_Normalize(Vec3s a) {
	f32 mgn = Vec3s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec3s_MulVal(a, 0.0f);
	else
		return Vec3s_DivVal(a, mgn);
}

Vec4s Vec4s_Normalize(Vec4s a) {
	f32 mgn = Vec4s_Magnitude(a);
	
	if (mgn == 0.0f)
		return Vec4s_MulVal(a, 0.0f);
	else
		return Vec4s_DivVal(a, mgn);
}

Vec2f Vec2f_LineSegDir(Vec2f a, Vec2f b) {
	return Vec2f_Normalize(Vec2f_Sub(b, a));
}

Vec3f Vec3f_LineSegDir(Vec3f a, Vec3f b) {
	return Vec3f_Normalize(Vec3f_Sub(b, a));
}

Vec4f Vec4f_LineSegDir(Vec4f a, Vec4f b) {
	return Vec4f_Normalize(Vec4f_Sub(b, a));
}

Vec2s Vec2s_LineSegDir(Vec2s a, Vec2s b) {
	return Vec2s_Normalize(Vec2s_Sub(b, a));
}

Vec3s Vec3s_LineSegDir(Vec3s a, Vec3s b) {
	return Vec3s_Normalize(Vec3s_Sub(b, a));
}

Vec4s Vec4s_LineSegDir(Vec4s a, Vec4s b) {
	return Vec4s_Normalize(Vec4s_Sub(b, a));
}

Vec2f Vec2f_Project(Vec2f a, Vec2f b) {
	f32 ls = Vec2f_MagnitudeSQ(b);
	
	return Vec2f_MulVal(b, Vec2f_Dot(b, a) / ls);
}

Vec3f Vec3f_Project(Vec3f a, Vec3f b) {
	f32 ls = Vec3f_MagnitudeSQ(b);
	
	return Vec3f_MulVal(b, Vec3f_Dot(b, a) / ls);
}

Vec4f Vec4f_Project(Vec4f a, Vec4f b) {
	f32 ls = Vec4f_MagnitudeSQ(b);
	
	return Vec4f_MulVal(b, Vec4f_Dot(b, a) / ls);
}

Vec2s Vec2s_Project(Vec2s a, Vec2s b) {
	f32 ls = Vec2s_MagnitudeSQ(b);
	
	return Vec2s_MulVal(b, Vec2s_Dot(b, a) / ls);
}

Vec3s Vec3s_Project(Vec3s a, Vec3s b) {
	f32 ls = Vec3s_MagnitudeSQ(b);
	
	return Vec3s_MulVal(b, Vec3s_Dot(b, a) / ls);
}

Vec4s Vec4s_Project(Vec4s a, Vec4s b) {
	f32 ls = Vec4s_MagnitudeSQ(b);
	
	return Vec4s_MulVal(b, Vec4s_Dot(b, a) / ls);
}

Vec2f Vec2f_Invert(Vec2f a) {
	return Vec2f_MulVal(a, -1);
}

Vec3f Vec3f_Invert(Vec3f a) {
	return Vec3f_MulVal(a, -1);
}

Vec4f Vec4f_Invert(Vec4f a) {
	return Vec4f_MulVal(a, -1);
}

Vec2s Vec2s_Invert(Vec2s a) {
	return Vec2s_MulVal(a, -1);
}

Vec3s Vec3s_Invert(Vec3s a) {
	return Vec3s_MulVal(a, -1);
}

Vec4s Vec4s_Invert(Vec4s a) {
	return Vec4s_MulVal(a, -1);
}

Vec2f Vec2f_InvMod(Vec2f a) {
	Vec2f r;
	
	for (int i = 0; i < ArrCount(a.axis); i++)
		r.axis[i] = 1.0f - fabsf(a.axis[i]);
	
	return r;
}

Vec3f Vec3f_InvMod(Vec3f a) {
	Vec3f r;
	
	for (int i = 0; i < ArrCount(a.axis); i++)
		r.axis[i] = 1.0f - fabsf(a.axis[i]);
	
	return r;
}

Vec4f Vec4f_InvMod(Vec4f a) {
	Vec4f r;
	
	for (int i = 0; i < ArrCount(a.axis); i++)
		r.axis[i] = 1.0f - fabsf(a.axis[i]);
	
	return r;
}

bool Vec2f_IsNaN(Vec2f a) {
	for (int i = 0; i < ArrCount(a.axis); i++)
		if (isnan(a.axis[i])) true;
	
	return false;
}

bool Vec3f_IsNaN(Vec3f a) {
	for (int i = 0; i < ArrCount(a.axis); i++)
		if (isnan(a.axis[i])) true;
	
	return false;
}

bool Vec4f_IsNaN(Vec4f a) {
	for (int i = 0; i < ArrCount(a.axis); i++)
		if (isnan(a.axis[i])) true;
	
	return false;
}

f32 Vec2f_Cos(Vec2f a, Vec2f b) {
	f32 mp = Vec2f_Magnitude(a) * Vec2f_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec2f_Dot(a, b) / mp;
}

f32 Vec3f_Cos(Vec3f a, Vec3f b) {
	f32 mp = Vec3f_Magnitude(a) * Vec3f_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec3f_Dot(a, b) / mp;
}

f32 Vec4f_Cos(Vec4f a, Vec4f b) {
	f32 mp = Vec4f_Magnitude(a) * Vec4f_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec4f_Dot(a, b) / mp;
}

f32 Vec2s_Cos(Vec2s a, Vec2s b) {
	f32 mp = Vec2s_Magnitude(a) * Vec2s_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec2s_Dot(a, b) / mp;
}

f32 Vec3s_Cos(Vec3s a, Vec3s b) {
	f32 mp = Vec3s_Magnitude(a) * Vec3s_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec3s_Dot(a, b) / mp;
}

f32 Vec4s_Cos(Vec4s a, Vec4s b) {
	f32 mp = Vec4s_Magnitude(a) * Vec4s_Magnitude(b);
	
	if (IsZero(mp)) return 0.0f;
	
	return Vec4s_Dot(a, b) / mp;
}

Vec2f Vec2f_Reflect(Vec2f vec, Vec2f normal) {
	Vec2f negVec = Vec2f_Invert(vec);
	f32 vecDotNormal = Vec2f_Cos(negVec, normal);
	Vec2f normalScale = Vec2f_MulVal(normal, vecDotNormal);
	Vec2f nsVec = Vec2f_Add(normalScale, vec);
	
	return Vec2f_Add(negVec, Vec2f_MulVal(nsVec, 2.0f));
}

Vec3f Vec3f_Reflect(Vec3f vec, Vec3f normal) {
	Vec3f negVec = Vec3f_Invert(vec);
	f32 vecDotNormal = Vec3f_Cos(negVec, normal);
	Vec3f normalScale = Vec3f_MulVal(normal, vecDotNormal);
	Vec3f nsVec = Vec3f_Add(normalScale, vec);
	
	return Vec3f_Add(negVec, Vec3f_MulVal(nsVec, 2.0f));
}

Vec4f Vec4f_Reflect(Vec4f vec, Vec4f normal) {
	Vec4f negVec = Vec4f_Invert(vec);
	f32 vecDotNormal = Vec4f_Cos(negVec, normal);
	Vec4f normalScale = Vec4f_MulVal(normal, vecDotNormal);
	Vec4f nsVec = Vec4f_Add(normalScale, vec);
	
	return Vec4f_Add(negVec, Vec4f_MulVal(nsVec, 2.0f));
}

Vec2s Vec2s_Reflect(Vec2s vec, Vec2s normal) {
	Vec2s negVec = Vec2s_Invert(vec);
	f32 vecDotNormal = Vec2s_Cos(negVec, normal);
	Vec2s normalScale = Vec2s_MulVal(normal, vecDotNormal);
	Vec2s nsVec = Vec2s_Add(normalScale, vec);
	
	return Vec2s_Add(negVec, Vec2s_MulVal(nsVec, 2.0f));
}

Vec3s Vec3s_Reflect(Vec3s vec, Vec3s normal) {
	Vec3s negVec = Vec3s_Invert(vec);
	f32 vecDotNormal = Vec3s_Cos(negVec, normal);
	Vec3s normalScale = Vec3s_MulVal(normal, vecDotNormal);
	Vec3s nsVec = Vec3s_Add(normalScale, vec);
	
	return Vec3s_Add(negVec, Vec3s_MulVal(nsVec, 2.0f));
}

Vec4s Vec4s_Reflect(Vec4s vec, Vec4s normal) {
	Vec4s negVec = Vec4s_Invert(vec);
	f32 vecDotNormal = Vec4s_Cos(negVec, normal);
	Vec4s normalScale = Vec4s_MulVal(normal, vecDotNormal);
	Vec4s nsVec = Vec4s_Add(normalScale, vec);
	
	return Vec4s_Add(negVec, Vec4s_MulVal(nsVec, 2.0f));
}

Vec3f Vec3f_ClosestPointOnRay(Vec3f rayStart, Vec3f rayEnd, Vec3f lineStart, Vec3f lineEnd) {
	Vec3f rayNorm = Vec3f_LineSegDir(rayStart, rayEnd);
	Vec3f lineNorm = Vec3f_LineSegDir(lineStart, lineEnd);
	f32 ndot = Vec3f_Dot(rayNorm, lineNorm);
	
	if (fabsf(ndot) >= 1.0f - EPSILON) {
		// parallel
		
		return lineStart;
	} else if (IsZero(ndot)) {
		// perpendicular
		Vec3f mod = Vec3f_InvMod(rayNorm);
		Vec3f ls = Vec3f_Mul(lineStart, mod);
		Vec3f rs = Vec3f_Mul(rayStart, mod);
		Vec3f side = Vec3f_Project(Vec3f_Sub(rs, ls), lineNorm);
		
		return Vec3f_Add(lineStart, side);
	} else {
		Vec3f startDiff = Vec3f_Sub(lineStart, rayStart);
		Vec3f crossNorm = Vec3f_Normalize(Vec3f_Cross(lineNorm, rayNorm));
		Vec3f rejection = Vec3f_Sub(
			Vec3f_Sub(startDiff, Vec3f_Project(startDiff, rayNorm)),
			Vec3f_Project(startDiff, crossNorm)
		);
		f32 rejDot = Vec3f_Dot(lineNorm, Vec3f_Normalize(rejection));
		f32 distToLinePos;
		
		if (rejDot == 0.0f)
			return lineStart;
		
		distToLinePos = Vec3f_Magnitude(rejection) / rejDot;
		
		return Vec3f_Sub(lineStart, Vec3f_MulVal(lineNorm, distToLinePos));
	}
	
}

Vec3f Vec3f_ProjectAlong(Vec3f point, Vec3f lineA, Vec3f lineB) {
	Vec3f seg = Vec3f_LineSegDir(lineA, lineB);
	Vec3f proj = Vec3f_Project(Vec3f_Sub(point, lineA), seg);
	
	return Vec3f_Add(lineA, proj);
}

Quat Quat_New(f32 x, f32 y, f32 z, f32 w) {
	return (Quat) { x, y, z, w };
}

Quat Quat_Identity() {
	return (Quat) { .w = 1.0f };
}

Quat Quat_Sub(Quat a, Quat b) {
	return (Quat) {
			   a.x - b.x,
			   a.y - b.y,
			   a.z - b.z,
			   a.w - b.w,
	};
}

Quat Quat_Add(Quat a, Quat b) {
	return (Quat) {
			   a.x + b.x,
			   a.y + b.y,
			   a.z + b.z,
			   a.w + b.w,
	};
}

Quat Quat_Div(Quat a, Quat b) {
	return (Quat) {
			   a.x / b.x,
			   a.y / b.y,
			   a.z / b.z,
			   a.w / b.w,
	};
}

Quat Quat_Mul(Quat a, Quat b) {
	return (Quat) {
			   a.x* b.x,
				   a.y* b.y,
				   a.z* b.z,
				   a.w* b.w,
	};
}

Quat Quat_QMul(Quat a, Quat b) {
	return (Quat) {
			   (a.x * b.w) + (b.x * a.w) + (a.y * b.z) - (a.z * b.y),
			   (a.y * b.w) + (b.y * a.w) + (a.z * b.x) - (a.x * b.z),
			   (a.z * b.w) + (b.z * a.w) + (a.x * b.y) - (a.y * b.x),
			   (a.w * b.w) + (b.x * a.x) + (a.y * b.y) - (a.z * b.z),
	};
}

Quat Quat_SubVal(Quat q, float val) {
	return (Quat) {
			   q.x - val,
			   q.y - val,
			   q.z - val,
			   q.w - val,
	};
}

Quat Quat_AddVal(Quat q, float val) {
	return (Quat) {
			   q.x + val,
			   q.y + val,
			   q.z + val,
			   q.w + val,
	};
}

Quat Quat_DivVal(Quat q, float val) {
	return (Quat) {
			   q.x / val,
			   q.y / val,
			   q.z / val,
			   q.w / val,
	};
}

Quat Quat_MulVal(Quat q, float val) {
	return (Quat) {
			   q.x* val,
				   q.y* val,
				   q.z* val,
				   q.w* val,
	};
}

f32 Quat_MagnitudeSQ(Quat q) {
	return SQ(q.x) + SQ(q.y) + SQ(q.z) + SQ(q.w);
}

f32 Quat_Magnitude(Quat q) {
	return sqrtf(SQ(q.x) + SQ(q.y) + SQ(q.z) + SQ(q.w));
}

Quat Quat_Normalize(Quat q) {
	f32 mgn = Quat_Magnitude(q);
	
	if (mgn)
		return Quat_DivVal(q, mgn);
	return Quat_New(0, 0, 0, 0);
}

Quat Quat_LineSegDir(Vec3f a, Vec3f b) {
	Vec3f c = Vec3f_Cross(a, b);
	f32 d = Vec3f_Dot(a, b);
	Quat q = Quat_New(c.x, c.y, c.z, 1.0f + d);
	
	return Quat_Normalize(q);
}

Quat Quat_Invert(Quat q) {
	f32 mgnSq = Quat_MagnitudeSQ(q);
	
	if (mgnSq)
		return (Quat) {
				   q.x / -mgnSq,
				   q.y / -mgnSq,
				   q.z / -mgnSq,
				   q.w / mgnSq,
		};
	
	return Quat_New(0, 0, 0, 0);
}

bool Quat_Equals(Quat* qa, Quat* qb) {
	f32* a = &qa->x;
	f32* b = &qb->x;
	
	for (int i = 0; i < 4; i++)
		if (fabsf(*a - *b) > EPSILON)
			return false;
	return true;
}
