#ifndef __Z64MATRIX_H__
#define __Z64MATRIX_H__

#include <ExtLib.h>
#include <ExtGui/Math.h>

typedef enum {
	MTXMODE_NEW,
	MTXMODE_APPLY
} MtxMode;

typedef union StructBE {
	s32 m[4][4];
	struct StructBE {
		u16 intPart[4][4];
		u16 fracPart[4][4];
	};
} Mtx;

typedef float MtxF_t[4][4];
typedef union {
	MtxF_t mf;
	struct {
		float xx, yx, zx, wx,
			xy, yy, zy, wy,
			xz, yz, zz, wz,
			xw, yw, zw, ww;
	};
} MtxF;

extern MtxF* gMatrixStack;
extern MtxF* gCurrentMatrix;
extern const MtxF gMtxFClear;

void Matrix_Init();
void Matrix_Clear(MtxF* mf);
void Matrix_Push(void);
void Matrix_Pop(void);
void Matrix_Get(MtxF* dest);
void Matrix_Put(MtxF* src);
void Matrix_MultVec3f(Vec3f* src, Vec3f* dest);
void Matrix_Transpose(MtxF* mf);
void Matrix_Translate(f32 x, f32 y, f32 z, MtxMode mode);
void Matrix_Scale(f32 x, f32 y, f32 z, MtxMode mode);
void Matrix_RotateX(f32 x, MtxMode mode);
void Matrix_RotateY(f32 y, MtxMode mode);
void Matrix_RotateZ(f32 z, MtxMode mode);

void Matrix_MultVec3f_Ext(Vec3f* src, Vec3f* dest, MtxF* mf);
void Matrix_MultVec3fToVec4f_Ext(Vec3f* src, Vec4f* dest, MtxF* mf);
void Matrix_OrientVec3f_Ext(Vec3f* src, Vec3f* dest, MtxF* mf);
void Matrix_Mult(MtxF* mf, MtxMode mode);
void Matrix_MtxFCopy(MtxF* dest, MtxF* src);
void Matrix_ToMtxF(MtxF* mtx);
Mtx* Matrix_ToMtx(Mtx* dest);
Mtx* Matrix_NewMtx();
void Matrix_MtxToMtxF(Mtx* src, MtxF* dest);
Mtx* Matrix_MtxFToMtx(MtxF* src, Mtx* dest);
void Matrix_MtxFMtxFMult(MtxF* mfA, MtxF* mfB, MtxF* dest);
void Matrix_Projection(MtxF* mtx, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale);
void Matrix_Ortho(MtxF* mtx, f32 fovy, f32 aspect, f32 near, f32 far);
void Matrix_LookAt(MtxF* mf, Vec3f eye, Vec3f at, Vec3f up);

void Matrix_TranslateRotateZYX(Vec3f* translation, Vec3s* rotation);
void Matrix_RotateAToB(Vec3f* a, Vec3f* b, u8 mode);
void Matrix_MultVec4f_Ext(Vec4f* src, Vec4f* dest, MtxF* mtx);
s32 Matrix_Invert(MtxF* src, MtxF* dest);
void Matrix_Unproject(MtxF* modelViewMtx, MtxF* projMtx, Vec3f* src, Vec3f* dest, f32 w, f32 h);
void Matrix_Project(MtxF* modelViewMtx, MtxF* projMtx, Vec3f* src, Vec3f* dest, f32 w, f32 h);

#define Matrix_RotateX_s(x, mode) Matrix_RotateX(BinToRad(x), mode)
#define Matrix_RotateY_s(x, mode) Matrix_RotateY(BinToRad(x), mode)
#define Matrix_RotateZ_s(x, mode) Matrix_RotateZ(BinToRad(x), mode)

#define Matrix_RotateX_d(x, mode) Matrix_RotateX(DegToRad(x), mode)
#define Matrix_RotateY_d(x, mode) Matrix_RotateY(DegToRad(x), mode)
#define Matrix_RotateZ_d(x, mode) Matrix_RotateZ(DegToRad(x), mode)

#endif
