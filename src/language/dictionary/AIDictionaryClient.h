#pragma once

#include <memory>
#include <string>

#include "language/dictionary/IDictionaryClient.h"

namespace Image2Card::AI
{
  class ITextAIProvider;
}

namespace Image2Card::Language
{
  class ILanguage;
}

namespace Image2Card::Language::Dictionary
{

  class AIDictionaryClient : public IDictionaryClient
  {
public:

    AIDictionaryClient(std::shared_ptr<Image2Card::AI::ITextAIProvider> aiProvider,
                       const Image2Card::Language::ILanguage& language);
    ~AIDictionaryClient() override = default;

    [[nodiscard]] DictionaryEntry LookupWord(const std::string& word, const std::string& headword = "") override;

    [[nodiscard]] bool IsAvailable() const override;

private:

    std::shared_ptr<Image2Card::AI::ITextAIProvider> m_AIProvider;
    const Image2Card::Language::ILanguage& m_Language;
  };

} // namespace Image2Card::Language::Dictionary