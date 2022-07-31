#ifndef __EXT_COLLISION_H__
#define __EXT_COLLISION_H__

#include <ExtGui/Math.h>

typedef struct {
	Vec3f v[3];
	u8    cullBackface;
	u8    cullFrontface;
} Triangle;

typedef struct {
	Vec3f start;
	Vec3f end;
	f32   nearest;
} RayLine;

RayLine RayLine_New(Vec3f start, Vec3f end);
bool Col3D_LineVsTriangle(RayLine* ray, Triangle* tri, Vec3f* out, bool cullBackface, bool cullFrontface);

#endif
