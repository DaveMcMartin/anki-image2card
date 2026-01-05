#include "AITranslator.h"

#include <nlohmann/json.hpp>

#include "ai/ITextAIProvider.h"
#include "core/Logger.h"
#include "language/ILanguage.h"

namespace Image2Card::Language::Translation
{

  AITranslator::AITranslator(std::shared_ptr<AI::ITextAIProvider> aiProvider, const Language::ILanguage& language)
      : m_AIProvider(std::move(aiProvider))
      , m_Language(language)
  {}

  std::string AITranslator::Translate(const std::string& text)
  {
    if (!m_AIProvider || text.empty()) {
      return "";
    }

    try {
      nlohmann::json analysis = m_AIProvider->AnalyzeSentence(text, "", m_Language);

      if (analysis.contains("translation") && analysis["translation"].is_string()) {
        return analysis["translation"].get<std::string>();
      }

      AF_WARN("AI translation failed: no translation field in response");
      return "";
    } catch (const std::exception& e) {
      AF_ERROR("AI translation error: {}", e.what());
      return "";
    }
  }

  bool AITranslator::IsAvailable() const
  {
    return m_AIProvider != nullptr;
  }

  std::string AITranslator::GetProviderName() const
  {
    if (m_AIProvider) {
      return m_AIProvider->GetName();
    }
    return "Unknown";
  }

} // namespace Image2Card::Language::Translation