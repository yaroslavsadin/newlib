/* sf_sin_int.c

   Copyright (C) 2013 Embecosm
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Adapteva nor the names of its contributors may be
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

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>

#define N 17

/* Pascal's Triangle */
int p_tri[N+1][N+2];

void
init_p_tri (void)
{
  int i, j;

  for (i = 0; i <= N; i++)
    {
      p_tri[i][0] = 1;
      for (j = 1; j < i; j++)
	p_tri[i][j] = p_tri[i-1][j-1] + p_tri[i-1][j];
      p_tri[i][i] = 1;
    }
}

/* T[n][i] is the coefficient for x^i in Tn. */
long double T[N+1][N+1], Ta[N+1][N+1];

/* Calculate ordinary T[n].  */
void
init_T (void)
{
  int n, i;
  for (n = 0; n <= 1; n++)
    for (i = 0; i <= N; i++)
      T[n][i] = 0.;
  T[0][0] = 1.;
  T[1][1] = 1.;
  for (n = 1; n < N; n++)
    {
      T[n+1][0] = - T[n-1][0];
      for (i = 1; i <= N; i++)
	T[n+1][i] = 2.*T[n][i-1] - T[n-1][i];
    }
}

/* Perform affine transform of function represented by polynom P to polynom Pa:
   P(x) -> Pa(z) ; x = f*z + s */
void
transform_polynom (long double P[], long double Pa[],
                   long double f, long double s)
{
  int n, i, j;
  long double sn[N+1], fn[N+1];

  sn[0] = fn[0] = 1.;
  for (n = 1; n <= N; n++)
    {
      fn[n] = fn[n-1] * f;
      sn[n] = sn[n-1] * s;
    }
  for (i = 0; i <= N; i++)
    Pa[i] = 0.;
  for (i = 0; i <= N; i++)
    for (j = 0; j <= i; j++)
      Pa[j] += P[i] * p_tri[i][j] * sn[i-j] * fn[j];
}

/* Perform affine transform of chebychev polynoms for the purpose of minimizing
   the maximum error in the interval (a,b) */
void
transform_chebyshev (long double a, long double b)
{
  int n, i, j;
  long double f, s, sn[N+1], fn[N+1];

  /* Transform T[n].  */
  /* T(x) -> Ta(z) ; x = s + f*z */
  f = 2./(b-a);
  s = -1. - a * f;
  for (n = 0; n <= N; n++)
    transform_polynom (T[n], Ta[n], f, s);
}

static long double S[N+1], Sa[N+1];

void
init_sin (void)
{
  int i;
  long double f;

  for (i = 1, f = 1.; i <= N; i+= 2, f *= (1-i)*i)
    S[i] = 1./f;
}

long double pi = 3.141592653589793238462643383279L;

/* Approximate P to a Polynom of grade n,
   (proximately) minimizing maximum absolute error in the interval (a,b) .  */
void
chebyshev_approx (long double P[], long double a, long double b, int n)
{
  int i, j;

  transform_chebyshev (a, b);
  for (i = N; i > n; i--)
    {
      long double f = P[i] / Ta[i][i];
      for (j = 0; j <= i; j++)
	P[j] -= Ta[i][j] * f;
    }
}

int
main (void)
{

  int i;
  long double a, b;
  long double start, step, base, grade;

  init_p_tri ();
  init_T ();
  init_sin ();
  /* Approximate sin close to zero without transform.  */
  assert (S[0] == 0);
  for (i = 0; i < N; i++)
    Sa[i] = S[i+1];
  Sa[N] = 0.;
  a = 1./16;
  chebyshev_approx (Sa, -a, a, 3);
  assert (fabs (Sa[4]) + fabs (Sa[3]) + fabs (Sa[1]) < 1e-20);
  printf ("%Le %Le Err %Le\n", Sa[2], Sa[0],
1-(((Sa[2])*a* a + Sa[0])* a) / sinl (a));
  printf ("0x%x\n", (unsigned)(-Sa[2] * (4ULL << 32)));

#ifdef SMALL
  step = 1./16;
  start = step*.33;
  grade = 4;
#elif 0
  step = 1./64;
  start = step*.5;
  grade = 3;
#else
  step = 1./64;
  start = step*.5;
  grade = 3;
#endif
  /* Special treatment for values near zero:
     By keeping the function centered around 0 and uneven, and dividing
     the polynom by x before approximation, it will be
     approximated with good relative error.  */
  printf ("   %Le %Le %Le %Le\n", S[3], S[2], S[1], S[0]);
  transform_polynom (S, Sa, pi, 0.);
  assert (Sa[0] == 0);
  for (i = 0; i < N; i++)
    Sa[i] = Sa[i+1];
  Sa[N] = 0.;
  a = start;
  chebyshev_approx (Sa, -a, a, 3);
  if (Sa[4] || Sa[3] || Sa[1])
    fprintf (stderr, "Sa[4]: %Le Sa[3]: %Le Sa[1]: %Le\n", Sa[4], Sa[3], Sa[1]);
  assert (fabs (Sa[4]) + fabs (Sa[3]) + fabs (Sa[1]) < 1e-20);
  printf ("%Le %Le Err %Le\n", Sa[2], Sa[0],
1-(((((Sa[4]*0+Sa[3])*a+Sa[2])*a + Sa[1])* a + Sa[0])* a) / sinl (a*pi));
  printf ("%lx %lx %x\n", (unsigned long)(-Sa[2]*0x20000000+0.5), (unsigned long)(Sa[0]*0x40000000+0.5), 0xdeadbeef);

  puts ("const uint32_t __sf_sin_pi_tab[] = {");
  /* Calculate main table, for sin(pi/step/2..pi/2).  */
  for (a = start; a < 0.5; a = b)
    {
      long double ea, eb, err;

      if (a != start)
	b = a + step;
      else
	for (b = step; b <= start; b += step);
      if (b > 1.)
	b = 1.;
#define BASE0 0
      if (BASE0 || a == start)
	base = 0.;
      else
	base = a;
      transform_polynom (S, Sa, pi, base*pi);
      chebyshev_approx (Sa, a - base, b - base, grade);
      ea = 1-(((((Sa[5]*0+Sa[4])*(a-base)+Sa[3])*(a-base) + Sa[2])* (a-base) + Sa[1])* (a-base) + Sa[0]) / sinl (a*pi);
      eb = 1-(((((Sa[5]*0+Sa[4])*(b-base)+Sa[3])*(b-base) + Sa[2])* (b-base) + Sa[1])* (b-base) + Sa[0]) / sinl (b*pi);
      err = abs (ea) >= abs (eb) ? ea : eb;
      if (grade < 4)
	{
	  unsigned w0, w1, w2, w3;
	  assert (Sa[4] == 0);
	  printf ("/* %Le %Le %Le %Le Err %Le*/\n",
		  Sa[3], Sa[2], Sa[1], Sa[0], err);
	  w3 = -Sa[3] * (1 << 29);
	  w2 = -Sa[2] * (1 << 28);
	  w1 =  Sa[1] * (1 << 30);
	  w0 =  Sa[0] * (1ULL << 32);
	  printf ("\t0x%x, 0x%x, 0x%x, 0x%x,\n", w3, w2, w1, w0);
	  if (b >= .5)
	    {
	      /* check that the maximum computed value doesn't overflow
		 the uint32_t bit result.  */
	      uint_fast32_t m, m32;
	      m = w3;
	      m32 = (1 << (32 - 5 - 2)) - 1;
	      m = ((uint64_t)m * m32 >> 32) + w2;
	      m32 <<= 3;
	      m = -((uint64_t)m * m32 >> 32) + w1;
	      m = ((uint64_t)m * m32 >> 32) + w0;
	      printf ("/* max m: %x */\n", m);
	    }
	}
      else
	{
	  printf ("/* %Le %Le %Le %Le %Le Err %Le*/\n", Sa[4], Sa[3], Sa[2], Sa[1], Sa[0],
		  err);
	  /* FIXME */ ;
	}
    }
  puts ("};");
  return 0;
}
