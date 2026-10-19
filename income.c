#include "income.h"
#include "carnum.h"

float income_car(void)
{
    return g_last_fee_x100 / 100.0f;
}