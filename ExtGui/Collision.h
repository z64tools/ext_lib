#ifndef __EXT_COLLISION_H__
#define __EXT_COLLISION_H__

#include <ExtGui/Math.h>

typedef struct {
	Vec3f v[3];
	Vec3f n[3];
} Triangle;

typedef struct {
	Vec3f v[3];
	Vec3f normal;
} TriPlane;

typedef struct {
	f32 minX, maxX;
	f32 minY, maxY;
	f32 minZ, maxZ;
} BoundingBox;

typedef struct {
	Vec3f a;
	Vec3f b;
} LineF;

bool Col3D_LineVsPoly(LineF line, Triangle* tri, u32 num, Vec3f* collision);

#endif