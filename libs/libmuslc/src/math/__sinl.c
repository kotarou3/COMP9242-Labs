/* @LICENSE(MUSLC_MIT) */

/* origin: FreeBSD /usr/src/lib/msun/ld80/k_sinl.c */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 * Copyright (c) 2008 Steven G. Kargl, David Schultz, Bruce D. Evans.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "libm.h"

#if LDBL_MANT_DIG == 64 && LDBL_MAX_EXP == 16384
/*
 * ld80 version of __sin.c.  See __sin.c for most comments.
 */
/*
 * Domain [-0.7854, 0.7854], range ~[-1.89e-22, 1.915e-22]
 * |sin(x)/x - s(x)| < 2**-72.1
 *
 * See __cosl.c for more details about the polynomial.
 */

static const long double
S1 = -0.166666666666666666671L;   /* -0xaaaaaaaaaaaaaaab.0p-66 */

static const double
S2 =  0.0083333333333333332,      /*  0x11111111111111.0p-59 */
S3 = -0.00019841269841269427,     /* -0x1a01a01a019f81.0p-65 */
S4 =  0.0000027557319223597490,   /*  0x171de3a55560f7.0p-71 */
S5 = -0.000000025052108218074604, /* -0x1ae64564f16cad.0p-78 */
S6 =  1.6059006598854211e-10,     /*  0x161242b90243b5.0p-85 */
S7 = -7.6429779983024564e-13,     /* -0x1ae42ebd1b2e00.0p-93 */
S8 =  2.6174587166648325e-15;     /*  0x179372ea0b3f64.0p-101 */

long double __sinl(long double x, long double y, int iy)
{
	long double z,r,v;

	z = x*x;
	v = z*x;
	r = S2+z*(S3+z*(S4+z*(S5+z*(S6+z*(S7+z*S8)))));
	if (iy == 0)
		return x+v*(S1+z*r);
	return x-((z*(0.5*y-v*r)-y)-v*S1);
}
#endif
