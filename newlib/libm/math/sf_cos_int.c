/* sf_cos_int.c

   Copyright (C) 2013 Embecosm
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Embecosm nor the names of its contributors may be
      used to endorse or promote products derived from this software without
      specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.                                               */

#include <inttypes.h>
#include "fdlibm.h"

#ifdef __IEEE_USE_SF_EMULATION

extern const uint32_t __sf_sin_pi_tab[32][4];
extern const uint32_t __one_by_2p31pi[];

typedef union { float f; uint32_t u; int32_t i; } sfunion;

float
cosf (float in_f)
{
  sfunion in = (sfunion) in_f;
  sfunion out;
  uint_fast32_t absu = in.u & 0x7fffffff;
  int chr = (absu >> 23) & 0xff;
  uint_fast32_t m = (absu | 0x00800000) << 8;
  uint_fast32_t m32;
  if (chr >= 127-4)
    {
      uint_fast32_t mtop;
      uint_fast32_t pr0, pr1, pr2;
      int lshift, rshift, nsign;
      int ix, nz;

      /* If the input is either +-inf or NaN, result is NaN.  */
      if (chr == 255)
	return nanf("");
/*
extract 64 bit part out of 128 (160?) bit 2^x/pi .
should need only one lowpart mul with high word and one highpart mul
with low word, (wrapping, unsigned) add together, to calculate 32 bit
precision result - ignoring leading integer part before even calculating it..
A further 32 bit highpart/lowpart might be needed if the result is
close to 0 / 1 to avoid precision loss due to cancellation (because of
the limited size of the mantissa and the irrationallity of the multiplicand,
it appears highly unlikely that it is possible to have a high relative error
after a further 32 have been added).
*/
      /* compute pi reprocipal appropriately scaled in pr{0,1,2}, then
         new m value in [0..1) interval.  */
      pr0 = __one_by_2p31pi[chr/32U-96/32];
      pr1 = __one_by_2p31pi[chr/32U-96/32+1];
      pr2 = __one_by_2p31pi[chr/32U-96/32+2];
      lshift = chr & 31;
      if (lshift)
	{
	  rshift = 32 - lshift;
	  pr0 = (pr0 << lshift) + (pr1 >> rshift);
	  pr1 = (pr1 << lshift) + (pr2 >> rshift);
	}
      mtop = (m * pr0) + (uint32_t)((uint64_t) m * pr1 >> 32);
      /* To calculate cos with sin polynoms, add the equivalent of pi/2.  */
      m32 = mtop += 0x40000000;
      /* sin (pi/2 + x) == sin (pi/2 - x) */
      if (mtop & 0x40000000)
	m32 = ~m32;
      m32 &= 0x7fffffff;
      nsign = __builtin_clrsb (m32);
      if (nsign > 4)
	{
	  uint_fast32_t pr3, mr;
	  uint_fast64_t m64;
	  if (lshift)
	    {
	      pr3 = __one_by_2p31pi[chr/32U-96/32+3];
	      pr2 = (pr2 << lshift) + (pr3 >> rshift);
	    }
	  m64 = ((((uint_fast64_t) mtop << 32) | (uint32_t) (m * pr1))
		 + ((uint64_t) m * pr2 >> 32));
	  mtop = m64 >> 32;
	  if (mtop & 0x40000000)
	    m64 = ~m64;
	  m64 &= 0x7fffffffffffffffULL;
	  if (nsign > 6)
	    {
	      /* If we overestimated nsign above, using the old value would
		 lead to overflow.  */
	      nsign = __builtin_clrsb ((int32_t) (m64 >> 32));
	      /* m32 = m64 >> (31 - nsign); */
	      m32 = ((uint32_t)(m64 >> 32) << nsign + 1
		     | (uint32_t)m64 >> 31 - nsign);
	      /* Use special polynom for low relative error.  */
	      /* Approx. (pi - pi^3/6 * m32^2) * m32, but with some
		 adjustment for left-out higher terms.  */
	      m = (0xa55ca0c7ULL * (uint32_t)(m64 >> 32) >> 32);
	      m = (uint64_t)m * (m32 >> nsign - 2) >> 32;
	      m = ((uint_fast64_t)(0xc90fdaa1UL - m) * m32) >> 32;
	      /* result is sgn(mtop) * m >> (nsign+30)  */
	      /* round / normalize.  */
	      mr = m + 0x7f;
	      if ((mr & 0x80000000) == 0)
		{
		  mr += m;
		  nsign++;
		}
	      mr >>= 8;
	      /* Add exponent.  */
	      mr += (0x7f - nsign) << 23;
	      /* And sign.  */
	      mr |= mtop & 0x80000000;
	      out.u = mr;
	      return out.f;
	    }
	  else
	    {
	      m32 = m64 >> 32;
	    }
	}
      ix = (m32 >> (32 - 5 - 2)) & (1 << 5) - 1;
      m = __sf_sin_pi_tab[ix][0];
      m32 &= (1 << (32 - 5 - 2)) - 1;
      m = ((uint64_t)m * m32 >> 32) + __sf_sin_pi_tab[ix][1];
      m32 <<= 3;
      m = -((uint64_t)m * m32 >> 32) + __sf_sin_pi_tab[ix][2];
      m =  ((uint64_t)m * m32 >> 32) + __sf_sin_pi_tab[ix][3];
      nz = __builtin_clz (m);
      m -= 0x80 >> nz;
      m >>= 8-nz;
      m++;
      /* Add exponent.  */
      m += (0x7d-nz) << 23;
      /* And sign.  */
      m |= mtop & 0x80000000;
      out.u = m;
      return out.f;
    }
  /* cos(x) ~= 1 - x^2/2 + x^4/24 == 1 - (x^2/2 - x^2/2 * x^2/2/6) */
  if (chr > 127-13)
    {
      m >>= 127 - chr;
      m32 = ((uint64_t)m * m) >> 32;
      m = 0x15555555ULL * m32 >> 32;
      m = (uint64_t)m * m32 >> 32;
      m32 += 0x3f;
      m32 -= m;
      m32 >>= 7;
      out.u = 0x3f800000 - m32;
    }
  else
    out.u = 0x3f800000;
  return out.f;
}

#endif /* __IEEE_USE_SF_EMULATION */
