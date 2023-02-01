#define GEO_VECTORGFX_C
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#include <ext_geogrid.h>

ElemAssets gAssets;

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

void VectorGfx_InitCommon() {
    if (gAssets.__initialized__)
        return;
    gAssets.__initialized__ = true;
    
    const Vec2f folder[] = {
        { 1.00,    1.50    },  { 6.00,  1.50   },
        { 6.00,    3.50    },  { 15.00, 3.50   },
        { 15.00,   5.50    },  { 1.00,  5.50   },
        { FLT_MAX, FLT_MAX },
        { 1.00,    6.50    },  { 15.00, 6.50   },
        { 15.00,   14.50   },  { 1.00,  14.50  },
    };
    const Vec2f cross[] = {
        { 4,  4   }, { 5,  4   },
        { 8,  7   }, { 11, 4   },
        { 12, 4   }, { 12, 5   },
        { 9,  8   }, { 12, 11  },
        { 12, 12  }, { 11, 12  },
        { 8,  9   }, { 5,  12  },
        { 4,  12  }, { 4,  11  },
        { 7,  8   }, { 4,  5   },
    };
    const Vec2f arrowParent[] = {
        { 1 + 6,  1   }, { 1 + 7,  1   },
        { 1 + 11, 5   }, { 1 + 11, 6   },
        { 1 + 10, 6   }, { 1 + 7,  3   },
        { 1 + 7,  13  }, { 1 + 8,  14  },
        { 1 + 14, 14  }, { 1 + 14, 15  },
        { 1 + 8,  15  }, { 1 + 6,  13  },
        { 1 + 6,  3   }, { 1 + 3,  6   },
        { 1 + 2,  6   }, { 1 + 2,  5   },
    };
    
    gAssets.folder = new(VectorGfx);
    gAssets.folder->num = ArrCount(folder);
    gAssets.folder->pos = qxf(memdup(folder, sizeof(folder)));
    
    gAssets.cross = new(VectorGfx);
    gAssets.cross->num = ArrCount(cross);
    gAssets.cross->pos = qxf(memdup(cross, sizeof(cross)));
    
    gAssets.arrowParent = new(VectorGfx);
    gAssets.arrowParent->num = ArrCount(arrowParent);
    gAssets.arrowParent->pos = qxf(memdup(arrowParent, sizeof(arrowParent)));
}

VectorGfx VectorGfx_New(VectorGfx* this, const void* data, f32 fidelity) {
    char* new = strdup(data);
    NSVGimage* img = nsvgParse(new, "px", 96.0f);
    VectorGfx vgfx = {};
    float tol = 1.f / fidelity;
    
    if (!this) this = &vgfx;
    
    vfree(new);
    
    if (!img) errr("Failed to parse data:\n%s", data);
    
    nested(void, AddPoint, (f32 x, f32 y)) {
        this->pos = realloc(this->pos, sizeof(Vec2f) * ++this->num);
        this->pos[this->num - 1].x = x;
        this->pos[this->num - 1].y = y;
    };
    
    nested(void, Bezier, (float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tol, int level)) {
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
            
            for (int i = 0; i < npts - 1; i += 3) {
                float* p = &pts[i * 2];
                Bezier(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], tol, 0);
            }
            
            if (closed) {
                AddPoint(pts[0], pts[1]);
            }
        }
        AddPoint(FLT_MAX, FLT_MAX);
    }
    
    nsvgDelete(img);
    
    return vgfx;
}

void VectorGfx_Free(VectorGfx* this) {
    vfree(this->pos);
    this->num = 0;
}
