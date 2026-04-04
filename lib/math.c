#include <math.h>

int pow(int base, int exponent)
{
    double res = 1;
    for (int i = 0; i < exponent; i++)
    {
        res = res * base;
    }
    return res;
}

float sqrt(float x)
{
    float result;
    __asm__ volatile(
        "flds %1\n"
        "fsqrt\n"
        "fstps %0\n"
        : "=m"(result)
        : "m"(x));
    return result;
}
