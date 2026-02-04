#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ui/UIComponent.h"

namespace Image2Card::API
{
  class AnkiConnectClient;
}

namespace Image2Card::Config
{
  class ConfigManager;
}

namespace Image2Card::AI
{
  class ITextAIProvider;
  class IAudioAIProvider;
} // namespace Image2Card::AI

namespace Image2Card::Language
{
  class ILanguage;

  namespace Services
  {
    class ILanguageService;
  }
} // namespace Image2Card::Language

namespace Image2Card::UI
{

  class ConfigurationSection : public UIComponent
  {
public:

    ConfigurationSection(API::AnkiConnectClient* ankiConnectClient,
                         Config::ConfigManager* configManager,
                         std::vector<std::shared_ptr<AI::ITextAIProvider>>* textAIProviders,
                         AI::IAudioAIProvider* audioAIProvider,
                         std::vector<std::unique_ptr<Language::Services::ILanguageService>>* languageServices,
                         std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                         Language::ILanguage** activeLanguage);
    ~ConfigurationSection() override;

    void Render() override;

    void SetOnConnectCallback(std::function<void()> callback) { m_OnConnectCallback = callback; }
    void SetOnTranslatorChangedCallback(std::function<void(const std::string&)> callback)
    {
      m_OnTranslatorChangedCallback = callback;
    }
    void SetOnNoteTypeOrDeckChangedCallback(std::function<void()> callback)
    {
      m_OnNoteTypeOrDeckChangedCallback = callback;
    }
    void SetOnAudioProviderChangedCallback(std::function<void(const std::string&)> callback)
    {
      m_OnAudioProviderChangedCallback = callback;
    }

    void SetAudioProvider(AI::IAudioAIProvider* provider) { m_AudioAIProvider = provider; }

    void RenderAnkiConnectTab();
    void RenderOCRTab();
    void RenderDictionaryTab();
    void RenderConfigurationTab();

private:

    // AnkiConnect State
    bool m_AnkiConnectConnected = false;
    std::string m_AnkiConnectError;

    API::AnkiConnectClient* m_AnkiConnectClient;
    Config::ConfigManager* m_ConfigManager;
    std::vector<std::shared_ptr<AI::ITextAIProvider>>* m_TextAIProviders;
    AI::IAudioAIProvider* m_AudioAIProvider;
    std::vector<std::unique_ptr<Language::Services::ILanguageService>>* m_LanguageServices;
    std::vector<std::unique_ptr<Language::ILanguage>>* m_Languages;
    Language::ILanguage** m_ActiveLanguage;

    std::function<void()> m_OnConnectCallback;
    std::function<void(const std::string&)> m_OnTranslatorChangedCallback;
    std::function<void()> m_OnNoteTypeOrDeckChangedCallback;
    std::function<void(const std::string&)> m_OnAudioProviderChangedCallback;
  };

} // namespace Image2Card::UI
