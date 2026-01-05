#pragma once

#include <string>

#include "language/translation/ITranslator.h"

namespace Image2Card::Language::Translation
{

  class NoneTranslator : public ITranslator
  {
public:

    NoneTranslator() = default;
    ~NoneTranslator() override = default;

    [[nodiscard]] std::string Translate(const std::string& text) override { return ""; }

    [[nodiscard]] bool IsAvailable() const override { return true; }
  };

} // namespace Image2Card::Language::Translation