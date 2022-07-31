#include <ExtGui/Collision.h>

RayLine RayLine_New(Vec3f start, Vec3f end) {
	return (RayLine) {
		       start, end, FLT_MAX
	};
}

bool Col3D_LineVsTriangle(RayLine* ray, Triangle* tri, Vec3f* out, bool backface) {
	const f32 EPSILON = 0.0000001;
	Vec3f vertex0 = tri->v[0];
	Vec3f vertex1 = tri->v[1];
	Vec3f vertex2 = tri->v[2];
	Vec3f edge1, edge2, h, s, q;
	f32 a, f, u, v;
	Vec3f dir = Math_Vec3f_Normalize(Math_Vec3f_Sub(ray->end, ray->start));
	f32 dist = Math_Vec3f_DistXYZ(ray->start, ray->end);
	
	edge1 = Math_Vec3f_Sub(vertex1, vertex0);
	edge2 = Math_Vec3f_Sub(vertex2, vertex0);
	h = Math_Vec3f_Cross(dir, edge2);
	a = Math_Vec3f_Dot(edge1, h);
	if (!backface && a < 0)
		return false;
	if (a > -EPSILON && a < EPSILON)
		return false;  // This ray is parallel to this triangle.
	f = 1.0 / a;
	s = Math_Vec3f_Sub(ray->start, vertex0);
	u = f * Math_Vec3f_Dot(s, h);
	if (u < 0.0 || u > 1.0)
		return false;
	q = Math_Vec3f_Cross(s, edge1);
	v = f * Math_Vec3f_Dot(dir, q);
	if (v < 0.0 || u + v > 1.0)
		return false;
	// At this stage we can compute t to find out where the intersection point is on the line.
	f32 t = f * Math_Vec3f_Dot(edge2, q);
	
	if (t > EPSILON && t < dist && t < ray->nearest) { // ray intersection
		*out = Math_Vec3f_Add(ray->start, Math_Vec3f_MulVal(dir, t));
		ray->nearest = t;
		
		return true;
	} else                 // This means that there is a line intersection but not a ray intersection.
		return false;
}