#pragma once

#include <memory>

#include "ocr/IOCRProvider.h"

namespace Image2Card::AI
{
  class ITextAIProvider;
}

namespace Image2Card::Language
{
  class ILanguage;
}

namespace Image2Card::OCR
{

  class AIOCRProvider : public IOCRProvider
  {
public:

    AIOCRProvider(std::shared_ptr<AI::ITextAIProvider> aiProvider, const Language::ILanguage& language);
    ~AIOCRProvider() override = default;

    std::string GetName() const override;

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) override;

    bool IsInitialized() const override;

private:

    std::shared_ptr<AI::ITextAIProvider> m_AIProvider;
    const Language::ILanguage& m_Language;
  };

} // namespace Image2Card::OCR