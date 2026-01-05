#include "NoneTranslationService.h"

namespace Image2Card::Language::Services
{

  NoneTranslationService::NoneTranslationService()
  {
    m_Translator = std::make_shared<Translation::NoneTranslator>();
  }

  std::string NoneTranslationService::GetName() const
  {
    return "None";
  }

  std::string NoneTranslationService::GetId() const
  {
    return "none";
  }

  std::string NoneTranslationService::GetType() const
  {
    return "translator";
  }

  bool NoneTranslationService::IsAvailable() const
  {
    return true;
  }

  bool NoneTranslationService::RenderConfigurationUI()
  {
    return false;
  }

  void NoneTranslationService::LoadConfig(const nlohmann::json& config)
  {
    (void) config;
  }

  nlohmann::json NoneTranslationService::SaveConfig() const
  {
    return nlohmann::json::object();
  }

  std::shared_ptr<Translation::ITranslator> NoneTranslationService::GetTranslator() const
  {
    return m_Translator;
  }

} // namespace Image2Card::Language::Services