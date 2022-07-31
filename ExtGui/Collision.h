#ifndef __EXT_COLLISION_H__
#define __EXT_COLLISION_H__

#include <ExtGui/Math.h>

typedef struct {
	Vec3f v[3];
	Vec3f n[3];
} Triangle;

bool Col3D_LineVsTriangle(Vec3f posA, Vec3f posB, Triangle* tri, Vec3f* out, float* t, bool* isBackface);

#endif
