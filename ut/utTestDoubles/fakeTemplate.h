#pragma once

#include "fff.h"

#undef ERROR
#undef LPTR


FAKE_VALUE_FUNC1(uint32_t, Template, uint32_t);

void FakeTemplateReset()
{
    RESET_FAKE(Template);
}
