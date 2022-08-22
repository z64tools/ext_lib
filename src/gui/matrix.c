#include "ext_matrix.h"

#define    FTOFIX32(x) (long)((x) * (float)0x00010000)
#define    FIX32TOF(x) ((float)(x) * (1.0f / (float)0x00010000))

MtxF* gMatrixStack;
MtxF* gCurrentMatrix;
const MtxF gMtxFClear = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};

void Matrix_Init() {
	if (gCurrentMatrix)
		return;
	gCurrentMatrix = Calloc(20 * sizeof(MtxF));
	gMatrixStack = gCurrentMatrix;
}

void Matrix_Clear(MtxF* mf) {
	mf->xx = 1.0f;
	mf->yy = 1.0f;
	mf->zz = 1.0f;
	mf->ww = 1.0f;
	mf->yx = 0.0f;
	mf->zx = 0.0f;
	mf->wx = 0.0f;
	mf->xy = 0.0f;
	mf->zy = 0.0f;
	mf->wy = 0.0f;
	mf->xz = 0.0f;
	mf->yz = 0.0f;
	mf->wz = 0.0f;
	mf->xw = 0.0f;
	mf->yw = 0.0f;
	mf->zw = 0.0f;
}

void Matrix_Push(void) {
	Matrix_MtxFCopy(gCurrentMatrix + 1, gCurrentMatrix);
	gCurrentMatrix++;
}

void Matrix_Pop(void) {
	gCurrentMatrix--;
}

void Matrix_Get(MtxF* dest) {
	Assert(dest != NULL);
	Matrix_MtxFCopy(dest, gCurrentMatrix);
}

void Matrix_Put(MtxF* src) {
	Assert(src != NULL);
	Matrix_MtxFCopy(gCurrentMatrix, src);
}

void Matrix_Mult(MtxF* mf, MtxMode mode) {
	MtxF* cmf = gCurrentMatrix;
	
	if (mode == MTXMODE_APPLY) {
		Matrix_MtxFMtxFMult(cmf, mf, cmf);
	} else {
		Matrix_MtxFCopy(gCurrentMatrix, mf);
	}
}

void Matrix_MultVec3f_Ext(Vec3f* src, Vec3f* dest, MtxF* mf) {
	dest->x = mf->xw + (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	dest->y = mf->yw + (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	dest->z = mf->zw + (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
}

void Matrix_OrientVec3f_Ext(Vec3f* src, Vec3f* dest, MtxF* mf) {
	dest->x = mf->xx * src->x + mf->xy * src->y + mf->xz * src->z;
	dest->y = mf->yx * src->x + mf->yy * src->y + mf->yz * src->z;
	dest->z = mf->zx * src->x + mf->zy * src->y + mf->zz * src->z;
}

void Matrix_MultVec3fToVec4f_Ext(Vec3f* src, Vec4f* dest, MtxF* mf) {
	dest->x = mf->xw + (mf->xx * src->x + mf->xy * src->y + mf->xz * src->z);
	dest->y = mf->yw + (mf->yx * src->x + mf->yy * src->y + mf->yz * src->z);
	dest->z = mf->zw + (mf->zx * src->x + mf->zy * src->y + mf->zz * src->z);
	dest->w = mf->ww + (mf->wx * src->x + mf->wy * src->y + mf->wz * src->z);
}

void Matrix_MultVec3f(Vec3f* src, Vec3f* dest) {
	MtxF* cmf = gCurrentMatrix;
	
	dest->x = cmf->xw + (cmf->xx * src->x + cmf->xy * src->y + cmf->xz * src->z);
	dest->y = cmf->yw + (cmf->yx * src->x + cmf->yy * src->y + cmf->yz * src->z);
	dest->z = cmf->zw + (cmf->zx * src->x + cmf->zy * src->y + cmf->zz * src->z);
}

void Matrix_Transpose(MtxF* mf) {
	f32 temp;
	
	temp = mf->yx;
	mf->yx = mf->xy;
	mf->xy = temp;
	
	temp = mf->zx;
	mf->zx = mf->xz;
	mf->xz = temp;
	
	temp = mf->zy;
	mf->zy = mf->yz;
	mf->yz = temp;
}

void Matrix_Translate(f32 x, f32 y, f32 z, MtxMode mode) {
	MtxF* cmf = gCurrentMatrix;
	f32 tx;
	f32 ty;
	
	if (mode == MTXMODE_APPLY) {
		tx = cmf->xx;
		ty = cmf->xy;
		cmf->xw += tx * x + ty * y + cmf->xz * z;
		tx = cmf->yx;
		ty = cmf->yy;
		cmf->yw += tx * x + ty * y + cmf->yz * z;
		tx = cmf->zx;
		ty = cmf->zy;
		cmf->zw += tx * x + ty * y + cmf->zz * z;
		tx = cmf->wx;
		ty = cmf->wy;
		cmf->ww += tx * x + ty * y + cmf->wz * z;
	} else {
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xx = 1.0f;
		cmf->yy = 1.0f;
		cmf->zz = 1.0f;
		cmf->ww = 1.0f;
		cmf->xw = x;
		cmf->yw = y;
		cmf->zw = z;
	}
}

void Matrix_Scale(f32 x, f32 y, f32 z, MtxMode mode) {
	MtxF* cmf = gCurrentMatrix;
	
	if (mode == MTXMODE_APPLY) {
		cmf->xx *= x;
		cmf->yx *= x;
		cmf->zx *= x;
		cmf->xy *= y;
		cmf->yy *= y;
		cmf->zy *= y;
		cmf->xz *= z;
		cmf->yz *= z;
		cmf->zz *= z;
		cmf->wx *= x;
		cmf->wy *= y;
		cmf->wz *= z;
	} else {
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->ww = 1.0f;
		cmf->xx = x;
		cmf->yy = y;
		cmf->zz = z;
	}
}

void Matrix_RotateX(f32 x, MtxMode mode) {
	MtxF* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (x != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(x);
			cos = cosf(x);
			
			temp1 = cmf->xy;
			temp2 = cmf->xz;
			cmf->xy = temp1 * cos + temp2 * sin;
			cmf->xz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->yy;
			temp2 = cmf->yz;
			cmf->yy = temp1 * cos + temp2 * sin;
			cmf->yz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->zy;
			temp2 = cmf->zz;
			cmf->zy = temp1 * cos + temp2 * sin;
			cmf->zz = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->wy;
			temp2 = cmf->wz;
			cmf->wy = temp1 * cos + temp2 * sin;
			cmf->wz = temp2 * cos - temp1 * sin;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (x != 0) {
			sin = sinf(x);
			cos = cosf(x);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->yx = 0.0f;
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->xx = 1.0f;
		cmf->ww = 1.0f;
		cmf->yy = cos;
		cmf->zz = cos;
		cmf->zy = sin;
		cmf->yz = -sin;
	}
}

void Matrix_RotateY(f32 y, MtxMode mode) {
	MtxF* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (y != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(y);
			cos = cosf(y);
			
			temp1 = cmf->xx;
			temp2 = cmf->xz;
			cmf->xx = temp1 * cos - temp2 * sin;
			cmf->xz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->yx;
			temp2 = cmf->yz;
			cmf->yx = temp1 * cos - temp2 * sin;
			cmf->yz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->zx;
			temp2 = cmf->zz;
			cmf->zx = temp1 * cos - temp2 * sin;
			cmf->zz = temp1 * sin + temp2 * cos;
			
			temp1 = cmf->wx;
			temp2 = cmf->wz;
			cmf->wx = temp1 * cos - temp2 * sin;
			cmf->wz = temp1 * sin + temp2 * cos;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (y != 0) {
			sin = sinf(y);
			cos = cosf(y);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->yx = 0.0f;
		cmf->wx = 0.0f;
		cmf->xy = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->yy = 1.0f;
		cmf->ww = 1.0f;
		cmf->xx = cos;
		cmf->zz = cos;
		cmf->zx = -sin;
		cmf->xz = sin;
	}
}

void Matrix_RotateZ(f32 z, MtxMode mode) {
	MtxF* cmf;
	f32 sin;
	f32 cos;
	f32 temp1;
	f32 temp2;
	
	if (mode == MTXMODE_APPLY) {
		if (z != 0) {
			cmf = gCurrentMatrix;
			
			sin = sinf(z);
			cos = cosf(z);
			
			temp1 = cmf->xx;
			temp2 = cmf->xy;
			cmf->xx = temp1 * cos + temp2 * sin;
			cmf->xy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->yx;
			temp2 = cmf->yy;
			cmf->yx = temp1 * cos + temp2 * sin;
			cmf->yy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->zx;
			temp2 = cmf->zy;
			cmf->zx = temp1 * cos + temp2 * sin;
			cmf->zy = temp2 * cos - temp1 * sin;
			
			temp1 = cmf->wx;
			temp2 = cmf->wy;
			cmf->wx = temp1 * cos + temp2 * sin;
			cmf->wy = temp2 * cos - temp1 * sin;
		}
	} else {
		cmf = gCurrentMatrix;
		
		if (z != 0) {
			sin = sinf(z);
			cos = cosf(z);
		} else {
			sin = 0.0f;
			cos = 1.0f;
		}
		
		cmf->zx = 0.0f;
		cmf->wx = 0.0f;
		cmf->zy = 0.0f;
		cmf->wy = 0.0f;
		cmf->xz = 0.0f;
		cmf->yz = 0.0f;
		cmf->wz = 0.0f;
		cmf->xw = 0.0f;
		cmf->yw = 0.0f;
		cmf->zw = 0.0f;
		cmf->zz = 1.0f;
		cmf->ww = 1.0f;
		cmf->xx = cos;
		cmf->yy = cos;
		cmf->yx = sin;
		cmf->xy = -sin;
	}
}

void Matrix_MtxFCopy(MtxF* dest, MtxF* src) {
	dest->xx = src->xx;
	dest->yx = src->yx;
	dest->zx = src->zx;
	dest->wx = src->wx;
	dest->xy = src->xy;
	dest->yy = src->yy;
	dest->zy = src->zy;
	dest->wy = src->wy;
	dest->xx = src->xx;
	dest->yx = src->yx;
	dest->zx = src->zx;
	dest->wx = src->wx;
	dest->xy = src->xy;
	dest->yy = src->yy;
	dest->zy = src->zy;
	dest->wy = src->wy;
	dest->xz = src->xz;
	dest->yz = src->yz;
	dest->zz = src->zz;
	dest->wz = src->wz;
	dest->xw = src->xw;
	dest->yw = src->yw;
	dest->zw = src->zw;
	dest->ww = src->ww;
	dest->xz = src->xz;
	dest->yz = src->yz;
	dest->zz = src->zz;
	dest->wz = src->wz;
	dest->xw = src->xw;
	dest->yw = src->yw;
	dest->zw = src->zw;
	dest->ww = src->ww;
}

void Matrix_ToMtxF(MtxF* mtx) {
	Matrix_MtxFCopy(mtx, gCurrentMatrix);
}

void Matrix_MtxToMtxF(Mtx* src, MtxF* dest) {
	u16* m1 = (void*)((u8*)src);
	u16* m2 = (void*)((u8*)src + 0x20);
	
	dest->xx = ((m1[0] << 0x10) | m2[0]) * (1 / 65536.0f);
	dest->yx = ((m1[1] << 0x10) | m2[1]) * (1 / 65536.0f);
	dest->zx = ((m1[2] << 0x10) | m2[2]) * (1 / 65536.0f);
	dest->wx = ((m1[3] << 0x10) | m2[3]) * (1 / 65536.0f);
	dest->xy = ((m1[4] << 0x10) | m2[4]) * (1 / 65536.0f);
	dest->yy = ((m1[5] << 0x10) | m2[5]) * (1 / 65536.0f);
	dest->zy = ((m1[6] << 0x10) | m2[6]) * (1 / 65536.0f);
	dest->wy = ((m1[7] << 0x10) | m2[7]) * (1 / 65536.0f);
	dest->xz = ((m1[8] << 0x10) | m2[8]) * (1 / 65536.0f);
	dest->yz = ((m1[9] << 0x10) | m2[9]) * (1 / 65536.0f);
	dest->zz = ((m1[10] << 0x10) | m2[10]) * (1 / 65536.0f);
	dest->wz = ((m1[11] << 0x10) | m2[11]) * (1 / 65536.0f);
	dest->xw = ((m1[12] << 0x10) | m2[12]) * (1 / 65536.0f);
	dest->yw = ((m1[13] << 0x10) | m2[13]) * (1 / 65536.0f);
	dest->zw = ((m1[14] << 0x10) | m2[14]) * (1 / 65536.0f);
	dest->ww = ((m1[15] << 0x10) | m2[15]) * (1 / 65536.0f);
}

Mtx* Matrix_MtxFToMtx(MtxF* src, Mtx* dest) {
	s32 temp;
	u16* m1 = (void*)((u8*)dest);
	u16* m2 = (void*)((u8*)dest + 0x20);
	
	temp = src->xx * 0x10000;
	m1[0] = (temp >> 0x10);
	m1[16 + 0] = temp & 0xFFFF;
	
	temp = src->yx * 0x10000;
	m1[1] = (temp >> 0x10);
	m1[16 + 1] = temp & 0xFFFF;
	
	temp = src->zx * 0x10000;
	m1[2] = (temp >> 0x10);
	m1[16 + 2] = temp & 0xFFFF;
	
	temp = src->wx * 0x10000;
	m1[3] = (temp >> 0x10);
	m1[16 + 3] = temp & 0xFFFF;
	
	temp = src->xy * 0x10000;
	m1[4] = (temp >> 0x10);
	m1[16 + 4] = temp & 0xFFFF;
	
	temp = src->yy * 0x10000;
	m1[5] = (temp >> 0x10);
	m1[16 + 5] = temp & 0xFFFF;
	
	temp = src->zy * 0x10000;
	m1[6] = (temp >> 0x10);
	m1[16 + 6] = temp & 0xFFFF;
	
	temp = src->wy * 0x10000;
	m1[7] = (temp >> 0x10);
	m1[16 + 7] = temp & 0xFFFF;
	
	temp = src->xz * 0x10000;
	m1[8] = (temp >> 0x10);
	m1[16 + 8] = temp & 0xFFFF;
	
	temp = src->yz * 0x10000;
	m1[9] = (temp >> 0x10);
	m2[9] = temp & 0xFFFF;
	
	temp = src->zz * 0x10000;
	m1[10] = (temp >> 0x10);
	m2[10] = temp & 0xFFFF;
	
	temp = src->wz * 0x10000;
	m1[11] = (temp >> 0x10);
	m2[11] = temp & 0xFFFF;
	
	temp = src->xw * 0x10000;
	m1[12] = (temp >> 0x10);
	m2[12] = temp & 0xFFFF;
	
	temp = src->yw * 0x10000;
	m1[13] = (temp >> 0x10);
	m2[13] = temp & 0xFFFF;
	
	temp = src->zw * 0x10000;
	m1[14] = (temp >> 0x10);
	m2[14] = temp & 0xFFFF;
	
	temp = src->ww * 0x10000;
	m1[15] = (temp >> 0x10);
	m2[15] = temp & 0xFFFF;
	
	return dest;
}

Mtx* Matrix_ToMtx(Mtx* dest) {
	return Matrix_MtxFToMtx(gCurrentMatrix, dest);
}

Mtx* Matrix_NewMtx() {
	return Matrix_ToMtx(xAlloc(sizeof(Mtx)));
}

void Matrix_MtxFMtxFMult(MtxF* mfA, MtxF* mfB, MtxF* dest) {
	f32 cx;
	f32 cy;
	f32 cz;
	f32 cw;
	//---ROW1---
	f32 rx = mfA->xx;
	f32 ry = mfA->xy;
	f32 rz = mfA->xz;
	f32 rw = mfA->xw;
	
	//--------
	
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->xx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->xy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->xz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->xw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW2---
	rx = mfA->yx;
	ry = mfA->yy;
	rz = mfA->yz;
	rw = mfA->yw;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->yx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->yy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->yz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->yw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW3---
	rx = mfA->zx;
	ry = mfA->zy;
	rz = mfA->zz;
	rw = mfA->zw;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->zx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->zy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->zz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->zw = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	//---ROW4---
	rx = mfA->wx;
	ry = mfA->wy;
	rz = mfA->wz;
	rw = mfA->ww;
	//--------
	cx = mfB->xx;
	cy = mfB->yx;
	cz = mfB->zx;
	cw = mfB->wx;
	dest->wx = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xy;
	cy = mfB->yy;
	cz = mfB->zy;
	cw = mfB->wy;
	dest->wy = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xz;
	cy = mfB->yz;
	cz = mfB->zz;
	cw = mfB->wz;
	dest->wz = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
	
	cx = mfB->xw;
	cy = mfB->yw;
	cz = mfB->zw;
	cw = mfB->ww;
	dest->ww = (rx * cx) + (ry * cy) + (rz * cz) + (rw * cw);
}

void Matrix_Projection(MtxF* mtx, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale) {
	f32 yscale;
	s32 row;
	s32 col;
	
	Matrix_Clear(mtx);
	
	fovy *= M_PI / 180.0;
	yscale = cosf(fovy / 2) / sinf(fovy / 2);
	mtx->mf[0][0] = yscale / aspect;
	mtx->mf[1][1] = yscale;
	mtx->mf[2][2] = (near + far) / (near - far);
	mtx->mf[2][3] = -1;
	mtx->mf[3][2] = 2 * near * far / (near - far);
	mtx->mf[3][3] = 0.0f;
	
	for (row = 0; row < 4; row++) {
		for (col = 0; col < 4; col++) {
			mtx->mf[row][col] *= scale;
		}
	}
}

static void Matrix_OrthoImpl(MtxF* mtx, f32 l, f32 r, f32 t, f32 b, f32 near, f32 far) {
	f32 rl = r - l;
	f32 tb = t - b;
	f32 fn = far - near;
	
	Matrix_Clear(mtx);
	
	mtx->xx = 2.0f / rl;
	mtx->yy = 2.0f / tb;
	mtx->zz = -2.0f / fn;
	
	mtx->xw = -(l + r) / rl;
	mtx->yw = -(t + b) / tb;
	mtx->zw = -(far + near) / fn;
	mtx->ww = 1.0f;
}

void Matrix_Ortho(MtxF* mtx, f32 fovy, f32 aspect, f32 near, f32 far) {
	f32 t = fovy / 2.0f;
	f32 r = t * aspect;
	
	Matrix_OrthoImpl(mtx, -r, r, t, -t, near, far);
}

void Matrix_LookAt(MtxF* mf, Vec3f eye, Vec3f at, Vec3f up) {
	f32 length;
	Vec3f look;
	Vec3f right;
	
	Matrix_Clear(mf);
	
	look.x = at.x - eye.x;
	look.y = at.y - eye.y;
	look.z = at.z - eye.z;
	length = -1.0 / sqrtf(SQ(look.x) + SQ(look.y) + SQ(look.z));
	look.x *= length;
	look.y *= length;
	look.z *= length;
	
	right.x = up.y * look.z - up.z * look.y;
	right.y = up.z * look.x - up.x * look.z;
	right.z = up.x * look.y - up.y * look.x;
	length = 1.0 / sqrtf(SQ(right.x) + SQ(right.y) + SQ(right.z));
	right.x *= length;
	right.y *= length;
	right.z *= length;
	
	up.x = look.y * right.z - look.z * right.y;
	up.y = look.z * right.x - look.x * right.z;
	up.z = look.x * right.y - look.y * right.x;
	length = 1.0 / sqrtf(SQ(up.x) + SQ(up.y) + SQ(up.z));
	up.x *= length;
	up.y *= length;
	up.z *= length;
	
	mf->mf[0][0] = right.x;
	mf->mf[1][0] = right.y;
	mf->mf[2][0] = right.z;
	mf->mf[3][0] = -(eye.x * right.x + eye.y * right.y + eye.z * right.z);
	
	mf->mf[0][1] = up.x;
	mf->mf[1][1] = up.y;
	mf->mf[2][1] = up.z;
	mf->mf[3][1] = -(eye.x * up.x + eye.y * up.y + eye.z * up.z);
	
	mf->mf[0][2] = look.x;
	mf->mf[1][2] = look.y;
	mf->mf[2][2] = look.z;
	mf->mf[3][2] = -(eye.x * look.x + eye.y * look.y + eye.z * look.z);
	
	mf->mf[0][3] = 0;
	mf->mf[1][3] = 0;
	mf->mf[2][3] = 0;
	mf->mf[3][3] = 1;
}

void Matrix_TranslateRotateZYX(Vec3f* translation, Vec3s* rotation) {
	MtxF* cmf = gCurrentMatrix;
	f32 sin = SinS(rotation->z);
	f32 cos = CosS(rotation->z);
	f32 temp1;
	f32 temp2;
	
	temp1 = cmf->xx;
	temp2 = cmf->xy;
	cmf->xw += temp1 * translation->x + temp2 * translation->y + cmf->xz * translation->z;
	cmf->xx = temp1 * cos + temp2 * sin;
	cmf->xy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->yx;
	temp2 = cmf->yy;
	cmf->yw += temp1 * translation->x + temp2 * translation->y + cmf->yz * translation->z;
	cmf->yx = temp1 * cos + temp2 * sin;
	cmf->yy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->zx;
	temp2 = cmf->zy;
	cmf->zw += temp1 * translation->x + temp2 * translation->y + cmf->zz * translation->z;
	cmf->zx = temp1 * cos + temp2 * sin;
	cmf->zy = temp2 * cos - temp1 * sin;
	
	temp1 = cmf->wx;
	temp2 = cmf->wy;
	cmf->ww += temp1 * translation->x + temp2 * translation->y + cmf->wz * translation->z;
	cmf->wx = temp1 * cos + temp2 * sin;
	cmf->wy = temp2 * cos - temp1 * sin;
	
	if (rotation->y != 0) {
		sin = SinS(rotation->y);
		cos = CosS(rotation->y);
		
		temp1 = cmf->xx;
		temp2 = cmf->xz;
		cmf->xx = temp1 * cos - temp2 * sin;
		cmf->xz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->yx;
		temp2 = cmf->yz;
		cmf->yx = temp1 * cos - temp2 * sin;
		cmf->yz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->zx;
		temp2 = cmf->zz;
		cmf->zx = temp1 * cos - temp2 * sin;
		cmf->zz = temp1 * sin + temp2 * cos;
		
		temp1 = cmf->wx;
		temp2 = cmf->wz;
		cmf->wx = temp1 * cos - temp2 * sin;
		cmf->wz = temp1 * sin + temp2 * cos;
	}
	
	if (rotation->x != 0) {
		sin = SinS(rotation->x);
		cos = CosS(rotation->x);
		
		temp1 = cmf->xy;
		temp2 = cmf->xz;
		cmf->xy = temp1 * cos + temp2 * sin;
		cmf->xz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->yy;
		temp2 = cmf->yz;
		cmf->yy = temp1 * cos + temp2 * sin;
		cmf->yz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->zy;
		temp2 = cmf->zz;
		cmf->zy = temp1 * cos + temp2 * sin;
		cmf->zz = temp2 * cos - temp1 * sin;
		
		temp1 = cmf->wy;
		temp2 = cmf->wz;
		cmf->wy = temp1 * cos + temp2 * sin;
		cmf->wz = temp2 * cos - temp1 * sin;
	}
}

void Matrix_RotateAToB(Vec3f* a, Vec3f* b, u8 mode) {
	MtxF mtx;
	Vec3f v;
	f32 frac;
	f32 c;
	Vec3f aN;
	Vec3f bN;
	
	// preliminary calculations
	aN = Math_Vec3f_Normalize(*a);
	bN = Math_Vec3f_Normalize(*b);
	c = Math_Vec3f_Dot(aN, bN);
	
	if (fabsf(1.0f + c) < EPSILON) {
		// The vectors are parallel and opposite. The transformation does not work for
		// this case, but simply inverting scale is sufficient in this situation.
		f32 d = Math_Vec3f_DistXYZ(aN, bN);
		
		if (d > 1.0f)
			Matrix_Scale(1.0f, 1.0f, 1.0f, mode);
		else
			Matrix_Scale(-1.0f, -1.0f, -1.0f, mode);
	} else {
		frac = 1.0f / (1.0f + c);
		v = Math_Vec3f_Cross(aN, bN);
		
		// fill mtx
		mtx.xx = 1.0f - frac * (SQ(v.y) + SQ(v.z));
		mtx.xy = v.z + frac * v.x * v.y;
		mtx.xz = -v.y + frac * v.x * v.z;
		mtx.xw = 0.0f;
		mtx.yx = -v.z + frac * v.x * v.y;
		mtx.yy = 1.0f - frac * (SQ(v.x) + SQ(v.z));
		mtx.yz = v.x + frac * v.y * v.z;
		mtx.yw = 0.0f;
		mtx.zx = v.y + frac * v.x * v.z;
		mtx.zy = -v.x + frac * v.y * v.z;
		mtx.zz = 1.0f - frac * (SQ(v.y) + SQ(v.x));
		mtx.zw = 0.0f;
		mtx.wx = 0.0f;
		mtx.wy = 0.0f;
		mtx.wz = 0.0f;
		mtx.ww = 1.0f;
		
		// apply to stack
		Matrix_Mult(&mtx, mode);
	}
}

void Matrix_MultVec4f_Ext(Vec4f* src, Vec4f* dest, MtxF* mtx) {
	dest->x = mtx->xx * src->x + mtx->xy * src->y + mtx->xz * src->z + mtx->xw * src->w;
	dest->y = mtx->yx * src->x + mtx->yy * src->y + mtx->yz * src->z + mtx->yw * src->w;
	dest->z = mtx->zx * src->x + mtx->zy * src->y + mtx->zz * src->z + mtx->zw * src->w;
	dest->w = mtx->wx * src->x + mtx->wy * src->y + mtx->wz * src->z + mtx->ww * src->w;
}

static int glhProjectf(float objx, float objy, float objz, float* modelview, float* projection, int* viewport, float* windowCoordinate) {
	// Transformation vectors
	float fTempo[8];
	
	// Modelview transform
	fTempo[0] = modelview[0] * objx + modelview[4] * objy + modelview[8] * objz + modelview[12]; // w is always 1
	fTempo[1] = modelview[1] * objx + modelview[5] * objy + modelview[9] * objz + modelview[13];
	fTempo[2] = modelview[2] * objx + modelview[6] * objy + modelview[10] * objz + modelview[14];
	fTempo[3] = modelview[3] * objx + modelview[7] * objy + modelview[11] * objz + modelview[15];
	// Projection transform, the final row of projection matrix is always [0 0 -1 0]
	// so we optimize for that.
	fTempo[4] = projection[0] * fTempo[0] + projection[4] * fTempo[1] + projection[8] * fTempo[2] + projection[12] * fTempo[3];
	fTempo[5] = projection[1] * fTempo[0] + projection[5] * fTempo[1] + projection[9] * fTempo[2] + projection[13] * fTempo[3];
	fTempo[6] = projection[2] * fTempo[0] + projection[6] * fTempo[1] + projection[10] * fTempo[2] + projection[14] * fTempo[3];
	fTempo[7] = -fTempo[2];
	// The result normalizes between -1 and 1
	if (fTempo[7]==0.0)    // The w value
		return 0;
	fTempo[7] = 1.0 / fTempo[7];
	// Perspective division
	fTempo[4] *= fTempo[7];
	fTempo[5] *= fTempo[7];
	fTempo[6] *= fTempo[7];
	// Window coordinates
	// Map x, y to range 0-1
	windowCoordinate[0] = (fTempo[4] * 0.5 + 0.5) * viewport[2] + viewport[0];
	windowCoordinate[1] = (fTempo[5] * 0.5 + 0.5) * viewport[3] + viewport[1];
	// This is only correct when glDepthRange(0.0, 1.0)
	windowCoordinate[2] = (1.0 + fTempo[6]) * 0.5; // Between 0 and 1
	
	return 1;
}

static void MultiplyMatrices4by4OpenGL_FLOAT(float* result, float* matrix1, float* matrix2) {
	result[0] = matrix1[0] * matrix2[0] + matrix1[4] * matrix2[1] + matrix1[8] * matrix2[2] + matrix1[12] * matrix2[3];
	result[1] = matrix1[1] * matrix2[0] + matrix1[5] * matrix2[1] + matrix1[9] * matrix2[2] + matrix1[13] * matrix2[3];
	result[2] = matrix1[2] * matrix2[0] + matrix1[6] * matrix2[1] + matrix1[10] * matrix2[2] + matrix1[14] * matrix2[3];
	result[3] = matrix1[3] * matrix2[0] + matrix1[7] * matrix2[1] + matrix1[11] * matrix2[2] + matrix1[15] * matrix2[3];
	result[4] = matrix1[0] * matrix2[4] + matrix1[4] * matrix2[5] + matrix1[8] * matrix2[6] + matrix1[12] * matrix2[7];
	result[5] = matrix1[1] * matrix2[4] + matrix1[5] * matrix2[5] + matrix1[9] * matrix2[6] + matrix1[13] * matrix2[7];
	result[6] = matrix1[2] * matrix2[4] + matrix1[6] * matrix2[5] + matrix1[10] * matrix2[6] + matrix1[14] * matrix2[7];
	result[7] = matrix1[3] * matrix2[4] + matrix1[7] * matrix2[5] + matrix1[11] * matrix2[6] + matrix1[15] * matrix2[7];
	result[8] = matrix1[0] * matrix2[8] + matrix1[4] * matrix2[9] + matrix1[8] * matrix2[10] + matrix1[12] * matrix2[11];
	result[9] = matrix1[1] * matrix2[8] + matrix1[5] * matrix2[9] + matrix1[9] * matrix2[10] + matrix1[13] * matrix2[11];
	result[10] = matrix1[2] * matrix2[8] + matrix1[6] * matrix2[9] + matrix1[10] * matrix2[10] + matrix1[14] * matrix2[11];
	result[11] = matrix1[3] * matrix2[8] + matrix1[7] * matrix2[9] + matrix1[11] * matrix2[10] + matrix1[15] * matrix2[11];
	result[12] = matrix1[0] * matrix2[12] + matrix1[4] * matrix2[13] + matrix1[8] * matrix2[14] + matrix1[12] * matrix2[15];
	result[13] = matrix1[1] * matrix2[12] + matrix1[5] * matrix2[13] + matrix1[9] * matrix2[14] + matrix1[13] * matrix2[15];
	result[14] = matrix1[2] * matrix2[12] + matrix1[6] * matrix2[13] + matrix1[10] * matrix2[14] + matrix1[14] * matrix2[15];
	result[15] = matrix1[3] * matrix2[12] + matrix1[7] * matrix2[13] + matrix1[11] * matrix2[14] + matrix1[15] * matrix2[15];
}

static void MultiplyMatrixByVector4by4OpenGL_FLOAT(float* resultvector, const float* matrix, const float* pvector) {
	resultvector[0] = matrix[0] * pvector[0] + matrix[4] * pvector[1] + matrix[8] * pvector[2] + matrix[12] * pvector[3];
	resultvector[1] = matrix[1] * pvector[0] + matrix[5] * pvector[1] + matrix[9] * pvector[2] + matrix[13] * pvector[3];
	resultvector[2] = matrix[2] * pvector[0] + matrix[6] * pvector[1] + matrix[10] * pvector[2] + matrix[14] * pvector[3];
	resultvector[3] = matrix[3] * pvector[0] + matrix[7] * pvector[1] + matrix[11] * pvector[2] + matrix[15] * pvector[3];
}

#define SWAP_ROWS_DOUBLE(a, b) do { double* _tmp = a; (a) = (b); (b) = _tmp; } while (0)
#define SWAP_ROWS_FLOAT(a, b)  do { float* _tmp = a; (a) = (b); (b) = _tmp; } while (0)
#define MAT(m, r, c)           (m)[(c) * 4 + (r)]

// This code comes directly from GLU except that it is for float
static int glhInvertMatrixf2(float* m, float* out) {
	float wtmp[4][8];
	float m0, m1, m2, m3, s;
	float* r0, * r1, * r2, * r3;
	
	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
	r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
	r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
	r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
	r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
	r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
	r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
	r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
	r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
	r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
	r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
	r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;
	/* choose pivot - or die */
	if (fabsf(r3[0]) > fabsf(r2[0]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (fabsf(r2[0]) > fabsf(r1[0]))
		SWAP_ROWS_FLOAT(r2, r1);
	if (fabsf(r1[0]) > fabsf(r0[0]))
		SWAP_ROWS_FLOAT(r1, r0);
	if (0.0 == r0[0])
		return 0;
	/* eliminate first variable */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}
	/* choose pivot - or die */
	if (fabsf(r3[1]) > fabsf(r2[1]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (fabsf(r2[1]) > fabsf(r1[1]))
		SWAP_ROWS_FLOAT(r2, r1);
	if (0.0 == r1[1])
		return 0;
	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}
	/* choose pivot - or die */
	if (fabsf(r3[2]) > fabsf(r2[2]))
		SWAP_ROWS_FLOAT(r3, r2);
	if (0.0 == r2[2])
		return 0;
	/* eliminate third variable */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
	r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
	/* last check */
	if (0.0 == r3[3])
		return 0;
	s = 1.0 / r3[3];       /* now back substitute row 3 */
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;
	m2 = r2[3];            /* now back substitute row 2 */
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
	r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
	r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
	r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
	m1 = r1[2];            /* now back substitute row 1 */
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
	r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
	m0 = r0[1];            /* now back substitute row 0 */
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];
	
	return 1;
}

static int glhUnProjectf(float winx, float winy, float winz, float* modelview, float* projection, int* viewport, float* objectCoordinate) {
	// Transformation matrices
	float m[16], A[16];
	float in[4], out[4];
	
	// Calculation for inverting a matrix, compute projection x modelview
	// and store in A[16]
	MultiplyMatrices4by4OpenGL_FLOAT(A, projection, modelview);
	// Now compute the inverse of matrix A
	if (glhInvertMatrixf2(A, m)==0)
		return 0;
	// Transformation of normalized coordinates between -1 and 1
	in[0] = (winx - (float)viewport[0]) / (float)viewport[2] * 2.0 - 1.0;
	in[1] = (winy - (float)viewport[1]) / (float)viewport[3] * 2.0 - 1.0;
	in[2] = 2.0 * winz - 1.0;
	in[3] = 1.0;
	// Objects coordinates
	MultiplyMatrixByVector4by4OpenGL_FLOAT(out, m, in);
	if (out[3]==0.0)
		return 0;
	out[3] = 1.0 / out[3];
	objectCoordinate[0] = out[0] * out[3];
	objectCoordinate[1] = out[1] * out[3];
	objectCoordinate[2] = out[2] * out[3];
	
	return 1;
}

void Matrix_Unproject(MtxF* modelViewMtx, MtxF* projMtx, Vec3f* src, Vec3f* dest, f32 w, f32 h) {
	s32 vp[] = {
		0, 0, w, h
	};
	
	src->y = h - src->y;
	glhUnProjectf(src->x, src->y, src->z, (float*)modelViewMtx, (float*)projMtx, vp, (float*)dest);
}

void Matrix_Project(MtxF* modelViewMtx, MtxF* projMtx, Vec3f* src, Vec3f* dest, f32 w, f32 h) {
	int vp[] = {
		0, 0, w, h
	};
	
	glhProjectf(src->x, src->y, src->z, (float*)modelViewMtx, (float*)projMtx, vp, (float*)dest);
	dest->y = h - dest->y;
}