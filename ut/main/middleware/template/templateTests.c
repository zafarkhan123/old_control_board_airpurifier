#include "fff.h"
#include "minunit.h"

// UUT
#include "middleware/template/template.h"
#include "fakeTemplateDriver.h"

#include <stdint.h>

DEFINE_FFF_GLOBALS;

void test_setup()
{
    FFF_RESET_HISTORY();
    FakeTemplateDriverReset();
}

void test_teardown()
{
}

MU_TEST(TemplateFunctionDummyTest)
{
    mu_assert(1 == Template(0));
}

MU_TEST_SUITE(TemplateTest)
{
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    MU_RUN_TEST(TemplateFunctionDummyTest);
}

int main(int argc, char *argv[])
{
    MU_RUN_SUITE(TemplateTest);
    MU_REPORT();
    return minunit_fail;
}
