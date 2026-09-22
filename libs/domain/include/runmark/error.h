#pragma once

#include <QString>

#include <stdexcept>

namespace runmark {

// The single failure contract. If this ever moves to std::expected, this is
// the only place that changes.
[[noreturn]] inline void fail(const QString& message)
{
    throw std::runtime_error(message.toStdString());
}

} // namespace runmark
