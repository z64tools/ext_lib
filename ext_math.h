#ifndef EXT_MATH_H
#define EXT_MATH_H

#include "ext_type.h"
#include "ext_macros.h"
#include <math.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static inline f32 Lerp(f32 x, f32 min, f32 max) {
    return min + (max - min) * x;
}

static inline f32 Normalize(f32 v, f32 min, f32 max) {
    return (v - min) / (max - min);
}

static inline s32 WrapS(s32 x, s32 min, s32 max) {
    s32 range = max - min;
    
    if (x < min)
        x += range * ((min - x) / range + 1);
    
    return min + (x - min) % range;
}

static inline f32 WrapF(f32 x, f32 min, f32 max) {
    f64 range = max - min;
    
    if (x < min)
        x += range * roundf((min - x) / range + 1);
    
    return min + fmodf((x - min), range);
}

static inline s32 PingPongS(s32 v, s32 min, s32 max) {
    min = WrapS(v, min, max * 2);
    if (min < max)
        return min;
    else
        return 2 * max - min;
}

static inline f32 PingPongF(f32 v, f32 min, f32 max) {
    min = WrapF(v, min, max * 2);
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

static inline f32 Remap(f32 v, f32 iMin, f32 iMax, f32 oMin, f32 oMax) {
    return Lerp(Normalize(v, iMin, iMax), oMin, oMax);
}

static inline f32 AccuracyF(f32 v, f32 mod) {
    return rint(rint(v * mod) / mod);
}

#pragma GCC diagnostic pop

#endif