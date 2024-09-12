#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>

void sqrt_avx2(int N, float initialGuess, float* values,float* output){
    __m256 x, guess, temp, error, kThreshold, temp2, zero;
    __m256 onef, threef, half;
    onef=_mm256_set1_ps(1.f);
    zero=_mm256_set1_ps(0.f);
    threef=_mm256_set1_ps(3.f);
    half=_mm256_set1_ps(0.5f);
    kThreshold=_mm256_set1_ps(0.00001f);
    for(int i=0;i<N;i+=8){
        x=_mm256_loadu_ps(values+i);
        guess=_mm256_set1_ps(initialGuess);
        temp=_mm256_mul_ps(guess, guess);
        temp=_mm256_mul_ps(temp, x);
        temp=_mm256_sub_ps(temp, onef);
        error=temp;

        __m256 mask=_mm256_cmp_ps(kThreshold, error, _CMP_LT_OQ);
        int flag=_mm256_movemask_ps(mask);
        while(flag!=0){
            temp=_mm256_mul_ps(x, guess);
            temp=_mm256_mul_ps(temp, guess);
            temp=_mm256_mul_ps(temp, guess);
            temp2=_mm256_mul_ps(guess, threef);
            temp=_mm256_sub_ps(temp2, temp);
            temp=_mm256_mul_ps(temp, half);
            //now temp=guess;

            guess=_mm256_blendv_ps(guess, temp, mask);

            temp2=_mm256_mul_ps(temp, temp);
            temp2=_mm256_mul_ps(temp, x);
            temp2=_mm256_sub_ps(temp2, onef);
            
            __m256 mask2=_mm256_cmp_ps(temp2, zero, _CMP_LT_OQ);
            temp=_mm256_sub_ps(zero, temp2);
            error=_mm256_blendv_ps(temp2, temp, mask2);

            mask=_mm256_cmp_ps(kThreshold, error, _CMP_LT_OQ);
            flag=_mm256_movemask_ps(mask);
        }
        _mm256_storeu_ps(output+i,guess);
    }
    return;
}
void sqrtSerial(int N,
                float initialGuess,
                float values[],
                float output[])
{

    static const float kThreshold = 0.00001f;

    for (int i=0; i<N; i++) {
       // printf("%d ",i);

        float x = values[i];
        float guess = initialGuess;

        float error = fabs(guess * guess * x - 1.f);

        while (error > kThreshold) {
            guess = (3.f * guess - x * guess * guess * guess) * 0.5f;
            error = fabs(guess * guess * x - 1.f);
        }

        output[i] = x * guess;
    }
}

