#ifndef EXT_MATH_H
#define EXT_MATH_H

#include "ext_type.h"
#include "ext_macros.h"
#include <math.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static inline f32 lerpf(f32 x, f32 min, f32 max) {
    return min + (max - min) * x;
}

static inline f32 normf(f32 v, f32 min, f32 max) {
    return (v - min) / (max - min);
}

static inline f64 lerpd(f64 x, f64 min, f64 max) {
    return min + (max - min) * x;
}

static inline f64 normd(f64 v, f64 min, f64 max) {
    return (v - min) / (max - min);
}

static inline s32 wrapi(s32 x, s32 min, s32 max) {
    s32 range = max - min;
    
    if (x < min)
        x += range * ((min - x) / range + 1);
    
    return min + (x - min) % range;
}

static inline f32 wrapf(f32 x, f32 min, f32 max) {
    f64 range = max - min;
    
    if (x < min)
        x += range * roundf((min - x) / range + 1);
    
    return min + fmodf((x - min), range);
}

static inline s32 pingpongi(s32 v, s32 min, s32 max) {
    min = wrapi(v, min, max * 2);
    if (min < max)
        return min;
    else
        return 2 * max - min;
}

static inline f32 pingpongf(f32 v, f32 min, f32 max) {
    min = wrapf(v, min, max * 2);
    if (min < max)
        return min;
    else
        return 2 * max - min;
}

static inline f32 Closest(f32 sample, f32 x, f32 y) {
    if ((fminf(sample, x) - fmaxf(sample, x) > fminf(sample, y) - fmaxf(sample, y)))
        return x;
    
    return y;
}

static inline f32 remapf(f32 v, f32 iMin, f32 iMax, f32 oMin, f32 oMax) {
    return lerpf(normf(v, iMin, iMax), oMin, oMax);
}

static inline f64 remapd(f64 v, f64 iMin, f64 iMax, f64 oMin, f64 oMax) {
    return lerpd(normd(v, iMin, iMax), oMin, oMax);
}

static inline f32 AccuracyF(f32 v, f32 mod) {
    return rint(rint(v * mod) / mod);
}

static inline f32 roundstepf(f32 v, f32 step) {
    return rintf(v * (1.0f / step)) * step;
}

static inline f32 invertf(f32 v) {
    return remapf(v, 0.0f, 1.0f, 1.0f, 0.0f);
}

#pragma GCC diagnostic pop

#endif