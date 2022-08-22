#ifndef EXT_COLLISION_H
#define EXT_COLLISION_H

#include "ext_math.h"
#include "ext_vector.h"

typedef struct {
	Vec3f v[3];
	u8    cullBackface;
	u8    cullFrontface;
} Triangle;

typedef struct {
	Triangle* head;
	u32       max;
	u32       num;
} TriBuffer;

typedef struct {
	Vec3f start;
	Vec3f end;
	f32   nearest;
} RayLine;

typedef struct {
	Vec3f pos;
	f32   r;
} Sphere;

typedef struct {
	Vec3f start;
	Vec3f end;
	f32   r;
} Cylinder;

void TriBuffer_Alloc(TriBuffer* this, u32 num);
void TriBuffer_Realloc(TriBuffer* this);
void TriBuffer_Free(TriBuffer* this);
RayLine RayLine_New(Vec3f start, Vec3f end);
bool Col3D_LineVsTriangle(RayLine* ray, Triangle* tri, Vec3f* outPos, Vec3f* outNor, bool cullBackface, bool cullFrontface);
bool Col3D_LineVsTriBuffer(RayLine* ray, TriBuffer* triBuf, Vec3f* outPos, Vec3f* outNor);
bool Col3D_LineVsCylinder(RayLine* ray, Cylinder* cyl, Vec3f* outPos);
bool Col3D_LineVsSphere(RayLine* ray, Sphere* sph, Vec3f* outPos);

#endif
