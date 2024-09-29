#include <libgte.h>

static int cos_table[ONE + 1];
static int sin_table[ONE + 1];

void InitTrigTables()
{
    for (int i=0; i<=ONE; i++)
    {
        cos_table[i] = ccos(i);
        sin_table[i] = csin(i);
    }
}

int clamp (int value, int min, int max)
{
    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}

int sin_lut(short angle)
{
    angle = clamp(angle, 0, ONE);
    return sin_table[angle];
}

int cos_lut(short angle)
{
    angle = clamp(angle, 0, ONE);
    return cos_table[angle];
}