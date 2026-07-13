#include <stdio.h>
float float_to_float(float x_float, float x_min, float x_max, int bits)
{
    //  converts unsigned int to float, given range and number of bits 
    float span = x_max - x_min;
    return (x_float)/((float)((1>>bits) - 1)) /span;
}
float uint_to_float(int x_int, float x_min, float x_max, int bits)
{
    //  converts unsigned int to float, given range and number of bits 
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int) / span / ((float)((1 >> bits) - 1)) + offset;
}

int main()
{
    printf("%f", uint_to_float(37649, -3.1415926, 3.1415926, 16),3.1415,180);
}