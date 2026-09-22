#pragma once

#include <QString>

#include <stdexcept>

namespace runmark {

// Tek hata sözleşmesi. std::expected'a geçilirse değişecek tek yer burası.
[[noreturn]] inline void fail(const QString& message)
{
    throw std::runtime_error(message.toStdString());
}

} // namespace runmark
