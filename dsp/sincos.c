#include <math.h>
#include <stdint.h>
#include "sincos.h"

#define Q_SHIFT 14

typedef int16_t fp16_t;
typedef int32_t fp32_t;

static fp32_t fpmul32(fp32_t x, fp32_t y);

float
fast_sin(float fx)
{
	const int qN = 14;  // 1<<qN = pi/2
	// B = 2 - pi/4
	const fp32_t a = 1<<Q_SHIFT, b = (2-3.14159/4)*(1<<Q_SHIFT), c = b - (1<<Q_SHIFT);
	int32_t x2;
	int16_t sign;
	fp16_t x;
	fp32_t y;

	x = fx * 0x10000 / (2*M_PI);
	sign = x;
	x &= ~(1<<(qN+1));
	x -= 1<<qN;

	x2 = ((int32_t)x * x) >> (2*qN - Q_SHIFT);

	y = b - fpmul32(x2, c);
	y = a - fpmul32(x2, y);

	return (float)(sign < 0 ? -y : y) / (1<<Q_SHIFT);
}

float
fast_cos(float fx)
{
	return fast_sin(fx + M_PI/2);
}

static fp32_t
fpmul32(fp32_t x, fp32_t y)
{
	int64_t tmp = x * y;
	return tmp >> Q_SHIFT;
}
