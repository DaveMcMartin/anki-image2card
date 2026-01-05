#pragma once

#include <memory>
#include <string>

#include "language/services/ILanguageService.h"
#include "language/translation/AITranslator.h"

namespace Image2Card::AI
{
  class ITextAIProvider;
}

namespace Image2Card::Language
{
  class ILanguage;
}

namespace Image2Card::Language::Services
{

  class AITranslationService : public ILanguageService
  {
public:

    AITranslationService(std::shared_ptr<AI::ITextAIProvider> aiProvider, const Language::ILanguage& language);
    ~AITranslationService() override = default;

    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] std::string GetId() const override;
    [[nodiscard]] std::string GetType() const override;
    [[nodiscard]] bool IsAvailable() const override;

    bool RenderConfigurationUI() override;

    void LoadConfig(const nlohmann::json& config) override;
    [[nodiscard]] nlohmann::json SaveConfig() const override;

    [[nodiscard]] std::shared_ptr<Translation::ITranslator> GetTranslator() const override;

private:

    std::shared_ptr<Translation::AITranslator> m_Translator;
    std::shared_ptr<AI::ITextAIProvider> m_AIProvider;
    const Language::ILanguage& m_Language;
  };

} // namespace Image2Card::Language::Services