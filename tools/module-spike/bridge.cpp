#include "bridge.h"

import runmark.spike.domain;

void Bridge::advance()
{
    m_value = runmark::spike::nextValue(m_value);
    emit valueChanged();
}
