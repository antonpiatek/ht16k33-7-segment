#include <unity.h>

void fuelGuage_is_initially_empty()
{
    TEST_ASSERT(1==1);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(fuelGuage_is_initially_empty);
    //RUN_ALL_TESTS();
    return UNITY_END();
}
