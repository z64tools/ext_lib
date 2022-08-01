#ifndef __EXT_COLLISION_H__
#define __EXT_COLLISION_H__

#include <ExtGui/Math.h>

typedef struct {
	Vec3f v[3];
	u8    cullBackface;
	u8    cullFrontface;
} Triangle;

typedef struct {
	Triangle* head;
	u32 max;
	u32 num;
} TriBuffer;

typedef struct {
	Vec3f start;
	Vec3f end;
	f32   nearest;
} RayLine;

void TriBuffer_Alloc(TriBuffer* this, u32 num);
void TriBuffer_Realloc(TriBuffer* this);
void TriBuffer_Free(TriBuffer* this);
RayLine RayLine_New(Vec3f start, Vec3f end);
bool Col3D_LineVsTriangle(RayLine* ray, Triangle* tri, Vec3f* out, bool cullBackface, bool cullFrontface);
bool Col3D_LineVsTriBuffer(RayLine* ray, TriBuffer* triBuf, Vec3f* out);

#endif
