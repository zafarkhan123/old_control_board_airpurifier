#include "fff.h"
#include "minunit.h"

// UUT
#include "driver/templateDriver/templateDriver.h"

#include <stdint.h>

DEFINE_FFF_GLOBALS;

void test_setup()
{
    FFF_RESET_HISTORY();
}

void test_teardown()
{
}

MU_TEST(TemplateDriverFunctionDummyTest)
{
    mu_assert(1 == TemplateDriver(0));
}

MU_TEST_SUITE(TemplateDriverTest)
{
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    MU_RUN_TEST(TemplateDriverFunctionDummyTest);
}

int main(int argc, char *argv[])
{
    MU_RUN_SUITE(TemplateDriverTest);
    MU_REPORT();
    return minunit_fail;
}
