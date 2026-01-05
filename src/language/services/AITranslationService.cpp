#include "AITranslationService.h"

#include "ai/ITextAIProvider.h"

namespace Image2Card::Language::Services
{

  AITranslationService::AITranslationService(std::shared_ptr<AI::ITextAIProvider> aiProvider,
                                             const Language::ILanguage& language)
      : m_AIProvider(aiProvider)
      , m_Language(language)
  {
    m_Translator = std::make_shared<Translation::AITranslator>(aiProvider, language);
  }

  std::string AITranslationService::GetName() const
  {
    if (m_AIProvider) {
      return m_AIProvider->GetName();
    }
    return "AI Translation";
  }

  std::string AITranslationService::GetId() const
  {
    if (m_AIProvider) {
      return m_AIProvider->GetId();
    }
    return "ai";
  }

  std::string AITranslationService::GetType() const
  {
    return "translator";
  }

  bool AITranslationService::IsAvailable() const
  {
    return m_Translator && m_Translator->IsAvailable();
  }

  bool AITranslationService::RenderConfigurationUI()
  {
    return false;
  }

  void AITranslationService::LoadConfig(const nlohmann::json& config)
  {
    (void) config;
  }

  nlohmann::json AITranslationService::SaveConfig() const
  {
    return nlohmann::json::object();
  }

  std::shared_ptr<Translation::ITranslator> AITranslationService::GetTranslator() const
  {
    return m_Translator;
  }

} // namespace Image2Card::Language::Services