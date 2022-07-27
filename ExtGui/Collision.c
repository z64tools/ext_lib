#include <ExtGui/Collision.h>

#define IS_ZERO(f) (fabsf(f) < 0.008f)

static TriPlane Col3D_TriPlane(Triangle* tri) {
	TriPlane tp;
	
	tp.normal = Math_Vec3f_Normalize(Math_Vec3f_Cross(Math_Vec3f_Sub(tri->v[1], tri->v[0]), Math_Vec3f_Sub(tri->v[1], tri->v[2])));
	
	tp.v[0] = tri->v[0];
	tp.v[1] = tri->v[1];
	tp.v[2] = tri->v[2];
	
	return tp;
}

static s32 Math3D_PointDistSqToLine2D(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32* lineLenSq) {
	static Vec3f perpendicularPoint;
	
	f32 perpendicularRatio;
	f32 xDiff;
	f32 distSq;
	f32 yDiff;
	s32 ret = false;
	
	xDiff = x2 - x1;
	yDiff = y2 - y1;
	distSq = SQ(xDiff) + SQ(yDiff);
	if (IS_ZERO(distSq)) {
		*lineLenSq = 0.0f;
		
		return false;
	}
	
	perpendicularRatio = (((x0 - x1) * xDiff) + (y0 - y1) * yDiff) / distSq;
	if (perpendicularRatio >= 0.0f && perpendicularRatio <= 1.0f) {
		ret = true;
	}
	perpendicularPoint.x = (xDiff * perpendicularRatio) + x1;
	perpendicularPoint.y = (yDiff * perpendicularRatio) + y1;
	*lineLenSq = SQ(perpendicularPoint.x - x0) + SQ(perpendicularPoint.y - y0);
	
	return ret;
}

static s32 Math3D_CirSquareVsTriSquare(f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2, f32 centerX, f32 centerY, f32 radius) {
	f32 minX;
	f32 maxX;
	f32 minY;
	f32 maxY;
	
	minX = maxX = x0;
	minY = maxY = y0;
	
	if (x1 < minX) {
		minX = x1;
	} else if (maxX < x1) {
		maxX = x1;
	}
	
	if (y1 < minY) {
		minY = y1;
	} else if (maxY < y1) {
		maxY = y1;
	}
	
	if (x2 < minX) {
		minX = x2;
	} else if (maxX < x2) {
		maxX = x2;
	}
	
	if (y2 < minY) {
		minY = y2;
	} else if (maxY < y2) {
		maxY = y2;
	}
	
	if ((minX - radius) <= centerX && (maxX + radius) >= centerX && (minY - radius) <= centerY &&
		(maxY + radius) >= centerY) {
		return true;
	}
	
	return false;
}

static s32 Math3D_TriChkPointParaXImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 y, f32 z, f32 detMax, f32 chkDist, f32 nx) {
	f32 detv0v1;
	f32 detv1v2;
	f32 detv2v0;
	f32 distToEdgeSq;
	f32 chkDistSq;
	
	if (!Math3D_CirSquareVsTriSquare(v0->y, v0->z, v1->y, v1->z, v2->y, v2->z, y, z, chkDist)) {
		return false;
	}
	
	chkDistSq = SQ(chkDist);
	
	if (((SQ(v0->y - y) + SQ(v0->z - z)) < chkDistSq) || ((SQ(v1->y - y) + SQ(v1->z - z)) < chkDistSq) ||
		((SQ(v2->y - y) + SQ(v2->z - z)) < chkDistSq)) {
		return true;
	}
	
	detv0v1 = ((v0->y - y) * (v1->z - z)) - ((v0->z - z) * (v1->y - y));
	detv1v2 = ((v1->y - y) * (v2->z - z)) - ((v1->z - z) * (v2->y - y));
	detv2v0 = ((v2->y - y) * (v0->z - z)) - ((v2->z - z) * (v0->y - y));
	
	if (((detv0v1 <= detMax) && (detv1v2 <= detMax) && (detv2v0 <= detMax)) ||
		((-detMax <= detv0v1) && (-detMax <= detv1v2) && (-detMax <= detv2v0))) {
		return true;
	}
	
	if (fabsf(nx) > 0.5f) {
		if (Math3D_PointDistSqToLine2D(y, z, v0->y, v0->z, v1->y, v1->z, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
		
		if (Math3D_PointDistSqToLine2D(y, z, v1->y, v1->z, v2->y, v2->z, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
		
		if (Math3D_PointDistSqToLine2D(y, z, v2->y, v2->z, v0->y, v0->z, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
	}
	
	return false;
}

static s32 Math3D_TriChkPointParaYImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 z, f32 x, f32 detMax, f32 chkDist, f32 ny) {
	f32 detv0v1;
	f32 detv1v2;
	f32 detv2v0;
	f32 distToEdgeSq;
	f32 chkDistSq;
	
	// first check if the point is within range of the triangle.
	if (!Math3D_CirSquareVsTriSquare(v0->z, v0->x, v1->z, v1->x, v2->z, v2->x, z, x, chkDist)) {
		return false;
	}
	
	// check if the point is within `chkDist` units of any vertex of the triangle.
	chkDistSq = SQ(chkDist);
	if (((SQ(v0->z - z) + SQ(v0->x - x)) < chkDistSq) || ((SQ(v1->z - z) + SQ(v1->x - x)) < chkDistSq) ||
		((SQ(v2->z - z) + SQ(v2->x - x)) < chkDistSq)) {
		return true;
	}
	
	// Calculate the determinant of each face of the triangle to the point.
	// If all the of determinants are within abs(`detMax`), return true.
	detv0v1 = ((v0->z - z) * (v1->x - x)) - ((v0->x - x) * (v1->z - z));
	detv1v2 = ((v1->z - z) * (v2->x - x)) - ((v1->x - x) * (v2->z - z));
	detv2v0 = ((v2->z - z) * (v0->x - x)) - ((v2->x - x) * (v0->z - z));
	
	if (((detMax >= detv0v1) && (detMax >= detv1v2) && (detMax >= detv2v0)) ||
		((-detMax <= detv0v1) && (-detMax <= detv1v2) && (-detMax <= detv2v0))) {
		return true;
	}
	
	if (fabsf(ny) > 0.5f) {
		// Do a check on each face of the triangle, if the point is within `chkDist` units return true.
		if (Math3D_PointDistSqToLine2D(z, x, v0->z, v0->x, v1->z, v1->x, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
		
		if (Math3D_PointDistSqToLine2D(z, x, v1->z, v1->x, v2->z, v2->x, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
		
		if (Math3D_PointDistSqToLine2D(z, x, v2->z, v2->x, v0->z, v0->x, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
	}
	
	return false;
};

static s32 Math3D_TriChkPointParaZImpl(Vec3f* v0, Vec3f* v1, Vec3f* v2, f32 x, f32 y, f32 detMax, f32 chkDist, f32 nz) {
	f32 detv0v1;
	f32 detv1v2;
	f32 detv2v0;
	f32 distToEdgeSq;
	f32 chkDistSq;
	
	if (!Math3D_CirSquareVsTriSquare(v0->x, v0->y, v1->x, v1->y, v2->x, v2->y, x, y, chkDist)) {
		return false;
	}
	
	chkDistSq = SQ(chkDist);
	
	if (((SQ(x - v0->x) + SQ(y - v0->y)) < chkDistSq) || ((SQ(x - v1->x) + SQ(y - v1->y)) < chkDistSq) ||
		((SQ(x - v2->x) + SQ(y - v2->y)) < chkDistSq)) {
		// Distance from any vertex to a point is less than chkDist
		return true;
	}
	
	detv0v1 = ((v0->x - x) * (v1->y - y)) - ((v0->y - y) * (v1->x - x));
	detv1v2 = ((v1->x - x) * (v2->y - y)) - ((v1->y - y) * (v2->x - x));
	detv2v0 = ((v2->x - x) * (v0->y - y)) - ((v2->y - y) * (v0->x - x));
	
	if (((detMax >= detv0v1) && (detMax >= detv1v2) && (detMax >= detv2v0)) ||
		((-detMax <= detv0v1) && (-detMax <= detv1v2) && (-detMax <= detv2v0))) {
		return true;
	}
	
	if (fabsf(nz) > 0.5f) {
		if (Math3D_PointDistSqToLine2D(x, y, v0->x, v0->y, v1->x, v1->y, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
		
		if (Math3D_PointDistSqToLine2D(x, y, v1->x, v1->y, v2->x, v2->y, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
		
		if (Math3D_PointDistSqToLine2D(x, y, v2->x, v2->y, v0->x, v0->y, &distToEdgeSq) && (distToEdgeSq < chkDistSq)) {
			return true;
		}
	}
	
	return false;
}

static void Math3D_PointOnInfiniteLine(Vec3f* v0, Vec3f* dir, f32 dist, Vec3f* ret) {
	ret->x = (dir->x * dist) + v0->x;
	ret->y = (dir->y * dist) + v0->y;
	ret->z = (dir->z * dist) + v0->z;
}

static void Math3D_LineSplitRatio(Vec3f* v0, Vec3f* v1, f32 ratio, Vec3f* ret) {
	Vec3f diff;
	
	diff = Math_Vec3f_Sub(*v1, *v0);
	Math3D_PointOnInfiniteLine(v0, &diff, ratio, ret);
}

bool Col3D_LineVsPoly(LineF line, Triangle* tri, u32 num, Vec3f* collision) {
	for (s32 i = 0; i < num; i++, tri++) {
		TriPlane plane = Col3D_TriPlane(tri);
		f32 da = Math_Vec3f_Dot(plane.normal, line.a);
		f32 db = Math_Vec3f_Dot(plane.normal, line.b);
		
		if ((da >= 0.0f && db >= 0.0f) || (da < 0.0f && db < 0.0f))
			continue;
		
		Math3D_LineSplitRatio(&line.a, &line.b, da / (da - db), collision);
		if (
			(fabsf(plane.normal.x) > 0.5f && Math3D_TriChkPointParaXImpl(&plane.v[0], &plane.v[1], &plane.v[2], collision->y, collision->z, 0.0f, 1.0f, plane.normal.x)) ||
			(fabsf(plane.normal.y) > 0.5f && Math3D_TriChkPointParaYImpl(&plane.v[0], &plane.v[1], &plane.v[2], collision->z, collision->x, 0.0f, 1.0f, plane.normal.y)) ||
			(fabsf(plane.normal.z) > 0.5f && Math3D_TriChkPointParaZImpl(&plane.v[0], &plane.v[1], &plane.v[2], collision->x, collision->y, 0.0f, 1.0f, plane.normal.z))
		)
			return true;
	}
	
	return false;
}