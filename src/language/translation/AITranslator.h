#pragma once

#include <memory>
#include <string>

#include "language/translation/ITranslator.h"

namespace Image2Card::AI
{
  class ITextAIProvider;
}

namespace Image2Card::Language
{
  class ILanguage;
}

namespace Image2Card::Language::Translation
{

  class AITranslator : public ITranslator
  {
public:

    AITranslator(std::shared_ptr<AI::ITextAIProvider> aiProvider, const Language::ILanguage& language);
    ~AITranslator() override = default;

    [[nodiscard]] std::string Translate(const std::string& text) override;

    [[nodiscard]] bool IsAvailable() const override;

    std::string GetProviderName() const;

private:

    std::shared_ptr<AI::ITextAIProvider> m_AIProvider;
    const Language::ILanguage& m_Language;
  };

} // namespace Image2Card::Language::Translation