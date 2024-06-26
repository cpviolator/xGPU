#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "xgpu.h"
#include "xgpu_info.h"

// Normally distributed random numbers with standard deviation of 2.5,
// quantized to integer values and saturated to the range -7.0 to +7.0.  For
// the fixed point case, the values are then converted to ints, scaled by 16
// (i.e. -112 to +112), and finally stored as signed chars.
void xgpuRandomComplex(ComplexInput* random_num, long long unsigned int length) {
  int i, j;
  double u1,u2,r,theta,a,b;
  double stddev=2.5;

#if defined(BENCHMARK) || defined(POWER_LOOP)
  int stride = NFREQUENCY*NTIME;
#else
  int stride = 1;
#endif
  int mini_length = length/stride;

  for(i=0; i<mini_length; i++){
    u1 = (rand() / (double)(RAND_MAX));
    u2 = (rand() / (double)(RAND_MAX));
    if(u1==0.0) u1=0.5/RAND_MAX;
    if(u2==0.0) u2=0.5/RAND_MAX;
    // Do Box-Muller transform
    r = stddev * sqrt(-2.0*log(u1));
    theta = 2*M_PI*u2;
    a = r * cos(theta);
    b = r * sin(theta);
    // Quantize (TODO: unbiased rounding?)
    a = round(a);
    b = round(b);
    // Saturate
    if(a >  7.0) a =  7.0;
    if(a < -7.0) a = -7.0;
    if(b >  7.0) b =  7.0;
    if(b < -7.0) b = -7.0;
#ifndef FIXED_POINT
    // Simulate 4 bit data that has been converted to floats
    // (i.e. {-7.0, -6.0, ..., +6.0, +7.0})
    //random_num[i] = ComplexInput( a, b );
    random_num[i].real = a;
    random_num[i].imag = b;
#else
    // Simulate 4 bit data that has been multipled by 16 (via left shift by 4;
    // could multiply by 18 to maximize range, but that might be more expensive
    // than left shift by 4).
    // (i.e. {-112, -96, -80, ..., +80, +96, +112})
    //random_num[i] = ComplexInput( ((int)a) << 4, ((int)b) << 4 );
    random_num[i].real = ((int)a) << 4;
    random_num[i].imag = ((int)b) << 4;

    // Uncomment next line to simulate all zeros for every input.
    // Interestingly, it does not give exactly zeros on the output.
    //random_num[i] = ComplexInput(0,0);
#endif
  }

  for (j=1; j<stride; j++) {
    memcpy(random_num+j*mini_length, random_num, sizeof(ComplexInput) * mini_length);
  }
}

void xgpuReorderMatrix(Complex *matrix) {

#if MATRIX_ORDER == REGISTER_TILE_TRIANGULAR_ORDER
  // reorder the matrix from REGISTER_TILE_TRIANGULAR_ORDER to TRIANGULAR_ORDER

  int f, i, rx, j, ry, pol1, pol2;
  size_t matLength = NFREQUENCY * ((NSTATION/2+1)*(NSTATION/4)*NPOL*NPOL*4) * (NPULSAR + 1);
  Complex *tmp = malloc(matLength * sizeof(Complex));
  memset(tmp, '0', matLength);

  for(f=0; f<NFREQUENCY; f++) {
    for(i=0; i<NSTATION/2; i++) {
      for (rx=0; rx<2; rx++) {
	for (j=0; j<=i; j++) {
	  for (ry=0; ry<2; ry++) {
	    int k = f*(NSTATION+1)*(NSTATION/2) + (2*i+rx)*(2*i+rx+1)/2 + 2*j+ry;
	    int l = f*4*(NSTATION/2+1)*(NSTATION/4) + (2*ry+rx)*(NSTATION/2+1)*(NSTATION/4) + i*(i+1)/2 + j;
	    for (pol1=0; pol1<NPOL; pol1++) {
	      for (pol2=0; pol2<NPOL; pol2++) {
		size_t tri_index = (k*NPOL+pol1)*NPOL+pol2;
		size_t reg_index = (l*NPOL+pol1)*NPOL+pol2;
		//tmp[tri_index] = 
		//  Complex(((float*)matrix)[reg_index], ((float*)matrix)[reg_index+matLength]);
#ifndef DP4A
		tmp[tri_index].real = ((float*)matrix)[reg_index];
		tmp[tri_index].imag = ((float*)matrix)[reg_index+matLength];
#else
		tmp[tri_index].real = ((int*)matrix)[reg_index];
		tmp[tri_index].imag = ((int*)matrix)[reg_index+matLength];
#endif
	      }
	    }
	  }
	}
      }
    }
  }
   
  memcpy(matrix, tmp, matLength*sizeof(Complex));

  free(tmp);

#elif MATRIX_ORDER == REAL_IMAG_TRIANGULAR_ORDER
  // reorder the matrix from REAL_IMAG_TRIANGULAR_ORDER to TRIANGULAR_ORDER
  
  int f, i, j, pol1, pol2;
  size_t matLength = NFREQUENCY * ((NSTATION+1)*(NSTATION/2)*NPOL*NPOL) * (NPULSAR + 1);
  Complex *tmp = malloc(matLength * sizeof(Complex));

  for(f=0; f<NFREQUENCY; f++){
    for(i=0; i<NSTATION; i++){
      for (j=0; j<=i; j++) {
	int k = f*(NSTATION+1)*(NSTATION/2) + i*(i+1)/2 + j;
        for (pol1=0; pol1<NPOL; pol1++) {
	  for (pol2=0; pol2<NPOL; pol2++) {
	    size_t index = (k*NPOL+pol1)*NPOL+pol2;
#ifndef DP4A
	    tmp[index].real = ((float*)matrix)[index];
	    tmp[index].imag = ((float*)matrix)[index+matLength];
#else
	    tmp[index].real = ((int*)matrix)[index];
	    tmp[index].imag = ((int*)matrix)[index+matLength];
#endif
	  }
	}
      }
    }
  }

  memcpy(matrix, tmp, matLength*sizeof(Complex));

  free(tmp);
#endif

  return;
}

#define zabs(z) (sqrt(z.real*z.real+z.imag*z.imag))

//check that GPU calculation matches the CPU
//
// verbose=0 means just print summary.
// verbsoe=1 means print each differing basline/channel.
// verbose=2 and array_h!=0 means print each differing baseline and each input
//           sample that contributed to it.
#ifndef FIXED_POINT
#define TOL 1e-12
#else
#define TOL 1e-5
#endif // FIXED_POINT
void xgpuCheckResult(Complex *gpu, Complex *cpu, int verbose, ComplexInput *array_h) {

  printf("Checking result (tolerance == %g)...\n", TOL); fflush(stdout);

  int errorCount=0;
  double error = 0.0;
  double maxError = 0.0;
  int i, j, pol1, pol2;
  long f, t;

  for(i=0; i<NSTATION; i++){
    for (j=0; j<=i; j++) {
      for (pol1=0; pol1<NPOL; pol1++) {
	for (pol2=0; pol2<NPOL; pol2++) {
	  for(f=0; f<NFREQUENCY; f++){
	    int k = f*(NSTATION+1)*(NSTATION/2) + i*(i+1)/2 + j;
	    int index = (k*NPOL+pol1)*NPOL+pol2;

#if defined(FIXED_POINT) && !defined(DP4A)
	    gpu[index].real = round(gpu[index].real);
	    gpu[index].imag = round(gpu[index].imag);
#endif

	    if(zabs(cpu[index]) == 0) {
	      error = zabs(gpu[index]);
	    } else {
              Complex delta;
              delta.real = cpu[index].real - gpu[index].real;
              delta.imag = cpu[index].imag - gpu[index].imag;
	      error = zabs(delta) / zabs(cpu[index]);
	    }
	    if(error > maxError) {
	      maxError = error;
	    }
	    if(error > TOL) {
              if(verbose > 0) {
#ifndef DP4A
                printf("%ld %d %d %d %d %d %d     %g  %g  %g  %g (%g %g)\n", f, i, j, k, pol1, pol2, index,
                       cpu[index].real, gpu[index].real, cpu[index].imag, gpu[index].imag, zabs(cpu[index]), zabs(gpu[index]));
#else
                printf("%3ld %3d %3d %4d %1d %1d %5d     %12d  %12d  %12d  %12d (%g %g)\n", f, i, j, k, pol1, pol2, index,
                       cpu[index].real, gpu[index].real, cpu[index].imag, gpu[index].imag, zabs(cpu[index]), zabs(gpu[index]));
#endif
                if(verbose > 1 && array_h) {
                  Complex sum;
                  sum.real = 0;
                  sum.imag = 0;
                  for(t=0; t<NTIME; t++) {
                    ComplexInput in0 = array_h[t*NFREQUENCY*NSTATION*2 + f*NSTATION*2 + i*2 + pol1];
                    ComplexInput in1 = array_h[t*NFREQUENCY*NSTATION*2 + f*NSTATION*2 + j*2 + pol2];
                    //Complex prod = convert(in0) * conj(convert(in1));
                    Complex prod;
                    prod.real = in0.real * in1.real + in0.imag * in1.imag;
                    prod.imag = in0.imag * in1.real - in0.real * in1.imag;

                    sum.real += prod.real;
                    sum.imag += prod.imag;
                    printf(" %4ld (%4g,%4g) (%4g,%4g) -> (%6g, %6g)\n", t,
                        //(float)real(in0), (float)imag(in0),
                        //(float)real(in1), (float)imag(in1),
                        //(float)real(prod), (float)imag(prod));
                        (float)in0.real, (float)in0.imag,
                        (float)in1.real, (float)in1.imag,
                        (float)prod.real, (float)prod.imag);
                  }
#ifndef DP4A
                  printf("                                 (%6g, %6g)\n", sum.real, sum.imag);
#else
                  printf("                                 (%6d, %6d)\n", sum.real, sum.imag);
#endif
                }
              }
	      errorCount++;
	    }
	  }
	}
      }
    }
  }

  if (errorCount) {
    printf("Outer product summation failed with %d deviations (max error %g)\n\n", errorCount, maxError);
  } else {
    printf("Outer product summation successful (max error %g)\n\n", maxError);
  }

}

// reorder the input array - separate real/imag and corner turn in time, depth 4
void xgpuSwizzleInput(ComplexInput *out, const ComplexInput *in) {
  printf("Swizzling input\n");

  signed char *o = (signed char*)out;
  const signed char *i = (signed char*)in;
  int t, f, s, p, c;

  for (t=0; t<NTIME_PIPE; t++) {
    for (f=0; f<NFREQUENCY; f++) {
      for(s=0; s<NSTATION; s++) {
	for (p=0; p<NPOL; p++) {
	  for (c=0; c<2; c++) {
	    o[((((t/4*NFREQUENCY+f)*NSTATION+s)*NPOL+p)*2+c)*4+t%4] =
	      i[( ( (t*NFREQUENCY+f)*NSTATION+s )*NPOL+p )*2 + c];
	  }
	}
      }
    }
  }

}

// Extracts the full matrix from the packed Hermitian form
void xgpuExtractMatrix(Complex *matrix, Complex *packed) {

  int f, i, j, pol1, pol2;
  for(f=0; f<NFREQUENCY; f++){
    for(i=0; i<NSTATION; i++){
      for (j=0; j<=i; j++) {
	int k = f*(NSTATION+1)*(NSTATION/2) + i*(i+1)/2 + j;
        for (pol1=0; pol1<NPOL; pol1++) {
	  for (pol2=0; pol2<NPOL; pol2++) {
	    int index = (k*NPOL+pol1)*NPOL+pol2;
	    matrix[(((f*NSTATION + i)*NSTATION + j)*NPOL + pol1)*NPOL+pol2].real = packed[index].real;
	    matrix[(((f*NSTATION + i)*NSTATION + j)*NPOL + pol1)*NPOL+pol2].imag = packed[index].imag;
	    matrix[(((f*NSTATION + j)*NSTATION + i)*NPOL + pol2)*NPOL+pol1].real =  packed[index].real;
	    matrix[(((f*NSTATION + j)*NSTATION + i)*NPOL + pol2)*NPOL+pol1].imag = -packed[index].imag;
	    //printf("%d %d %d %d %d %d %d\n",f,i,j,k,pol1,pol2,index);
	  }
	}
      }
    }
  }

}
