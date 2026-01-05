#pragma once

#include <memory>
#include <string>

#include "language/services/ILanguageService.h"
#include "language/translation/NoneTranslator.h"

namespace Image2Card::Language::Services
{

  class NoneTranslationService : public ILanguageService
  {
public:

    NoneTranslationService();
    ~NoneTranslationService() override = default;

    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] std::string GetId() const override;
    [[nodiscard]] std::string GetType() const override;
    [[nodiscard]] bool IsAvailable() const override;

    bool RenderConfigurationUI() override;

    void LoadConfig(const nlohmann::json& config) override;
    [[nodiscard]] nlohmann::json SaveConfig() const override;

    [[nodiscard]] std::shared_ptr<Translation::ITranslator> GetTranslator() const override;

private:

    std::shared_ptr<Translation::NoneTranslator> m_Translator;
  };

} // namespace Image2Card::Language::Services