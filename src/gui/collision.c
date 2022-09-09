#include "ext_collision.h"
#include "ext_matrix.h"

void TriBuffer_Alloc(TriBuffer* this, u32 num) {
    this->head = Calloc(sizeof(Triangle) * num);
    this->max = num;
    this->num = 0;
    
    Assert(this->head != NULL);
}

void TriBuffer_Realloc(TriBuffer* this) {
    this->max *= 2;
    this->head = Realloc(this->head, sizeof(Triangle) * this->max);
}

void TriBuffer_Free(TriBuffer* this) {
    Free(this->head);
    memset(this, 0, sizeof(*this));
}

RayLine RayLine_New(Vec3f start, Vec3f end) {
    return (RayLine) {
               start, end, FLT_MAX
    };
}

bool Col3D_LineVsTriangle(RayLine* ray, Triangle* tri, Vec3f* outPos, Vec3f* outNor, bool cullBackface, bool cullFrontface) {
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
    if ((cullBackface && a < 0) || (cullFrontface && a > 0))
        return false;
    if (a > -EPSILON && a < EPSILON)
        return false;          // This ray is parallel to this triangle.
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
        if (outPos)
            *outPos = Math_Vec3f_Add(ray->start, Math_Vec3f_MulVal(dir, t));
        if (outNor)
            *outNor = h;
        ray->nearest = t;
        
        return true;
    } else                     // This means that there is a line intersection but not a ray intersection.
        return false;
}

bool Col3D_LineVsTriBuffer(RayLine* ray, TriBuffer* triBuf, Vec3f* outPos, Vec3f* outNor) {
    Triangle* tri = triBuf->head;
    s32 r = 0;
    
    for (s32 i = 0; i < triBuf->num; i++, tri++) {
        if (Col3D_LineVsTriangle(ray, tri, outPos, outNor, tri->cullBackface, tri->cullFrontface))
            r = true;
    }
    
    return r;
}

bool Col3D_LineVsCylinder(RayLine* ray, Cylinder* cyl, Vec3f* outPos) {
    Vec3f rA, rB;
    Vec3f cA, cB;
    Vec3f cyln = Math_Vec3f_LineSegDir(cyl->start, cyl->end);
    Vec3f up = { 0, 1, 0 };
    Vec3f ip;
    Vec3f o;
    
    Matrix_Push();
    
    Matrix_RotateAToB(&cyln, &up, MTXMODE_NEW);
    Matrix_MultVec3f(&ray->start, &rA);
    Matrix_MultVec3f(&ray->end, &rB);
    Matrix_MultVec3f(&cyl->start, &cA);
    Matrix_MultVec3f(&cyl->end, &cB);
    
    Matrix_Pop();
    
    ip = Math_Vec3f_ClosestPointOnRay(rA, rB, cA, cB);
    if (ip.y < fminf(cA.y, cB.y) || ip.y > fmaxf(cA.y, cB.y))
        return false;
    
    Sphere s = {
        cA, cyl->r
    };
    RayLine r = {
        rA, rB, ray->nearest
    };
    
    r.start.y = 0;
    r.end.y = 0;
    s.pos.y = 0;
    
    if (!Col3D_LineVsSphere(&r, &s, &o))
        return false;
    
    ray->nearest = r.nearest;
    if (outPos) {
        Vec3f out;
        
        out = o;
        out.y = ip.y;
        
        cyln = Math_Vec3f_Invert(cyln);
        Matrix_Push();
        Matrix_RotateAToB(&cyln, &up, MTXMODE_NEW);
        Matrix_MultVec3f(&out, outPos);
        Matrix_Pop();
    }
    
    return true;
}

bool Col3D_LineVsSphere(RayLine* ray, Sphere* sph, Vec3f* outPos) {
    Vec3f dir = Math_Vec3f_LineSegDir(ray->start, ray->end);
    f32 rayCylLen = Math_Vec3f_DistXYZ(ray->start, sph->pos);
    Vec3f npos = Math_Vec3f_Add(ray->start, Math_Vec3f_MulVal(dir, rayCylLen));
    f32 dist = Math_Vec3f_DistXYZ(sph->pos, npos);
    
    if (dist < sph->r && rayCylLen < ray->nearest) {
        ray->nearest = rayCylLen;
        if (outPos) {
            Vec3f nb = Math_Vec3f_LineSegDir(sph->pos, npos);
            
            *outPos = Math_Vec3f_Add(sph->pos, Math_Vec3f_MulVal(nb, dist));
        }
        
        return true;
    }
    
    return false;
}
