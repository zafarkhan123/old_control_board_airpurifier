#pragma once

#include "fff.h"

#undef ERROR
#undef LPTR


FAKE_VALUE_FUNC1(uint32_t, TemplateDriver, uint32_t);

void FakeTemplateDriverReset()
{
    RESET_FAKE(TemplateDriver);
}
