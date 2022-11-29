#include <ext_geogrid.h>
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

static float distPtSeg(float x, float y, float px, float py, float qx, float qy) {
    float pqx, pqy, dx, dy, d, t;
    
    pqx = qx - px;
    pqy = qy - py;
    dx = x - px;
    dy = y - py;
    d = pqx * pqx + pqy * pqy;
    t = pqx * dx + pqy * dy;
    if (d > 0) t /= d;
    if (t < 0) t = 0;
    else if (t > 1) t = 1;
    dx = px + t * pqx - x;
    dy = py + t * pqy - y;
    return dx * dx + dy * dy;
};

VectorGfx VectorGfx_New(VectorGfx* this, const void* data, f32 fidelity) {
    char* new = strdup(data);
    NSVGimage* img = nsvgParse(new, "px", 96.0f);
    VectorGfx vgfx = {};
    float tol = 1.f / fidelity;
    
    if (!this) this = &vgfx;
    
    Free(new);
    
    if (!img) printf_error("Failed to parse data:\n%s", data);
    
    Block(void, AddPoint, (f32 x, f32 y)) {
        this->pos = Realloc(this->pos, sizeof(Vec2f) * ++this->num);
        this->pos[this->num - 1].x = x;
        this->pos[this->num - 1].y = y;
    };
    
    Block(void, Bezier, (float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tol, int level)) {
        float x12, y12, x23, y23, x34, y34, x123, y123, x234, y234, x1234, y1234;
        float d;
        
        if (level > 12) return;
        
        x12 = (x1 + x2) * 0.5f;
        y12 = (y1 + y2) * 0.5f;
        x23 = (x2 + x3) * 0.5f;
        y23 = (y2 + y3) * 0.5f;
        x34 = (x3 + x4) * 0.5f;
        y34 = (y3 + y4) * 0.5f;
        x123 = (x12 + x23) * 0.5f;
        y123 = (y12 + y23) * 0.5f;
        x234 = (x23 + x34) * 0.5f;
        y234 = (y23 + y34) * 0.5f;
        x1234 = (x123 + x234) * 0.5f;
        y1234 = (y123 + y234) * 0.5f;
        
        d = distPtSeg(x1234, y1234, x1, y1, x4, y4);
        if (d > tol * tol) {
            Bezier(x1, y1, x12, y12, x123, y123, x1234, y1234, tol, level + 1);
            Bezier(x1234, y1234, x234, y234, x34, y34, x4, y4, tol, level + 1);
        } else {
            AddPoint(x4, y4);
        }
    };
    
    for (NSVGshape* shape = img->shapes; shape; shape = shape->next) {
        for (NSVGpath* path = shape->paths; path; path = path->next) {
            float* pts = path->pts;
            int npts = path->npts;
            bool closed = path->closed;
            
            AddPoint(pts[0], pts[1]);
            
            for (s32 i = 0; i < npts - 1; i += 3) {
                float* p = &pts[i * 2];
                Bezier(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], tol, 0);
            }
            
            if (closed) {
                AddPoint(pts[0], pts[1]);
            }
        }
        AddPoint(FLT_MAX, FLT_MAX);
    }
    
    return vgfx;
}

void VectorGfx_Free(VectorGfx* this) {
    Free(this->pos);
    this->num = 0;
}
