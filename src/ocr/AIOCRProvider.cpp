#include "AIOCRProvider.h"

#include "ai/ITextAIProvider.h"
#include "language/ILanguage.h"

namespace Image2Card::OCR
{

  AIOCRProvider::AIOCRProvider(std::shared_ptr<AI::ITextAIProvider> aiProvider, const Language::ILanguage& language)
      : m_AIProvider(std::move(aiProvider))
      , m_Language(language)
  {}

  std::string AIOCRProvider::GetName() const
  {
    return m_AIProvider ? m_AIProvider->GetName() + " (AI)" : "AI OCR";
  }

  std::string AIOCRProvider::ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer)
  {
    if (!m_AIProvider) {
      return "";
    }

    return m_AIProvider->ExtractTextFromImage(imageBuffer, "image/webp", m_Language);
  }

  bool AIOCRProvider::IsInitialized() const
  {
    return m_AIProvider != nullptr;
  }

} // namespace Image2Card::OCR