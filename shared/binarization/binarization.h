#ifndef BINARIZATION_H
#define BINARIZATION_H

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define USE_STD_DEV 0
// #define BLACK_BRIGHT 0x50

#ifdef __cplusplus
extern "C" {
#endif

extern void threshold(unsigned char* src, unsigned char* out, uint_fast16_t width, uint_fast16_t height);

static inline double __cdecl sqr(double _x)
{
	return _x * _x;
}

#ifdef __cplusplus
}
#endif

#endif /*BINARIZATION_H*/
