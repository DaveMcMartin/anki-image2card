#include "ui/ConfigurationSection.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <thread>

#include "ai/IAudioAIProvider.h"
#include "ai/ITextAIProvider.h"
#include "api/AnkiConnectClient.h"
#include "config/ConfigManager.h"
#include "language/ILanguage.h"

namespace Image2Card::UI
{

  ConfigurationSection::ConfigurationSection(API::AnkiConnectClient* ankiConnectClient,
                                             Config::ConfigManager* configManager,
                                             std::vector<std::unique_ptr<AI::ITextAIProvider>>* textAIProviders,
                                             AI::ITextAIProvider** activeTextAIProvider,
                                             AI::IAudioAIProvider* audioAIProvider,
                                             std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                                             Language::ILanguage** activeLanguage)
      : m_AnkiConnectClient(ankiConnectClient)
      , m_ConfigManager(configManager)
      , m_TextAIProviders(textAIProviders)
      , m_ActiveTextAIProvider(activeTextAIProvider)
      , m_AudioAIProvider(audioAIProvider)
      , m_Languages(languages)
      , m_ActiveLanguage(activeLanguage)
  {}

  ConfigurationSection::~ConfigurationSection() {}

  void ConfigurationSection::Render()
  {
    if (ImGui::BeginTabBar("ConfigTabs")) {
      if (ImGui::BeginTabItem("AnkiConnect")) {
        RenderAnkiConnectTab();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Text AI")) {
        RenderTextAITab();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Audio AI")) {
        RenderAudioAITab();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("OCR")) {
        RenderOCRTab();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }

  void ConfigurationSection::RenderAnkiConnectTab()
  {
    ImGui::Spacing();
    ImGui::Text("AnkiConnect Configuration");
    ImGui::Separator();
    ImGui::Spacing();

    if (!m_ConfigManager)
      return;
    auto& config = m_ConfigManager->GetConfig();

    if (ImGui::InputText("URL", &config.AnkiConnectUrl)) {
      m_ConfigManager->Save();
    }

    ImGui::Spacing();

    if (ImGui::Button("Connect")) {
      m_AnkiConnectError.clear();
      if (m_AnkiConnectClient) {
        std::thread([this, url = config.AnkiConnectUrl]() {
          m_AnkiConnectClient->SetUrl(url);
          m_AnkiConnectConnected = m_AnkiConnectClient->Ping();
          if (m_AnkiConnectConnected) {
            if (m_OnConnectCallback) {
              m_OnConnectCallback();
            }
          } else {
            m_AnkiConnectError = "Connection failed. Ensure Anki is open and AnkiConnect is installed.";
          }
        }).detach();
      }
    }

    ImGui::SameLine();
    if (m_AnkiConnectConnected) {
      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected");
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");
    }

    if (!m_AnkiConnectError.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", m_AnkiConnectError.c_str());
    }
  }

  void ConfigurationSection::RenderTextAITab()
  {
    ImGui::Spacing();
    ImGui::Text("Text AI Provider Selection");
    ImGui::Separator();
    ImGui::Spacing();

    if (!m_TextAIProviders || !m_ActiveTextAIProvider || !m_ConfigManager)
      return;

    auto& config = m_ConfigManager->GetConfig();

    if (ImGui::BeginCombo("Provider", (*m_ActiveTextAIProvider)->GetName().c_str())) {
      for (auto& provider : *m_TextAIProviders) {
        bool isSelected = (provider.get() == *m_ActiveTextAIProvider);
        if (ImGui::Selectable(provider->GetName().c_str(), isSelected)) {
          *m_ActiveTextAIProvider = provider.get();
          config.TextProvider = provider->GetId();
          m_ConfigManager->Save();
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (*m_ActiveTextAIProvider) {
      ImGui::Text("Provider Configuration:");
      ImGui::Spacing();

      if ((*m_ActiveTextAIProvider)->RenderConfigurationUI()) {
        auto j = (*m_ActiveTextAIProvider)->SaveConfig();
        std::string providerId = (*m_ActiveTextAIProvider)->GetId();

        if (providerId == "xai") {
          if (j.contains("api_key"))
            config.TextApiKey = j["api_key"];
          if (j.contains("vision_model"))
            config.TextVisionModel = j["vision_model"];
          if (j.contains("sentence_model"))
            config.TextSentenceModel = j["sentence_model"];
          if (j.contains("available_models"))
            config.TextAvailableModels = j["available_models"].get<std::vector<std::string>>();
        } else if (providerId == "google") {
          if (j.contains("api_key"))
            config.GoogleApiKey = j["api_key"];
          if (j.contains("vision_model"))
            config.GoogleVisionModel = j["vision_model"];
          if (j.contains("sentence_model"))
            config.GoogleSentenceModel = j["sentence_model"];
          if (j.contains("available_models"))
            config.GoogleAvailableModels = j["available_models"].get<std::vector<std::string>>();
        }

        m_ConfigManager->Save();
      }
    }
  }

  void ConfigurationSection::RenderAudioAITab()
  {
    ImGui::Spacing();
    ImGui::Text("Audio AI Provider");
    ImGui::Separator();
    ImGui::Spacing();

    if (m_AudioAIProvider) {
      ImGui::Text("Provider: %s", m_AudioAIProvider->GetName().c_str());
      ImGui::Spacing();

      if (m_AudioAIProvider->RenderConfigurationUI()) {
        // Save changes to config
        if (m_ConfigManager) {
          auto j = m_AudioAIProvider->SaveConfig();
          auto& config = m_ConfigManager->GetConfig();

          if (j.contains("api_key"))
            config.AudioApiKey = j["api_key"];
          if (j.contains("voice_id"))
            config.AudioVoiceId = j["voice_id"];
          if (j.contains("audio_format"))
            config.AudioFormat = j["audio_format"];

          if (j.contains("available_voices")) {
            config.AudioAvailableVoices.clear();
            for (const auto& item : j["available_voices"]) {
              if (item.is_array() && item.size() == 2) {
                config.AudioAvailableVoices.push_back({item[0], item[1]});
              }
            }
          }

          m_ConfigManager->Save();
        }
      }
    }
  }

  void ConfigurationSection::RenderOCRTab()
  {
    ImGui::Spacing();
    ImGui::Text("OCR Method Selection");
    ImGui::Separator();
    ImGui::Spacing();

    if (!m_ConfigManager)
      return;

    auto& config = m_ConfigManager->GetConfig();

    ImGui::Text("Select OCR Method:");
    ImGui::Spacing();

    bool isAI = (config.OCRMethod == "AI");
    bool isTesseract = (config.OCRMethod == "Tesseract");

    if (ImGui::RadioButton("AI (Cloud-based)", isAI)) {
      config.OCRMethod = "AI";
      m_ConfigManager->Save();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Uses AI providers (Google Gemini or xAI) for OCR.\nRequires internet connection and API key.");
    }

    if (ImGui::RadioButton("Tesseract (Local)", isTesseract)) {
      config.OCRMethod = "Tesseract";
      m_ConfigManager->Save();
    }

    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Uses local Tesseract OCR.\nWorks offline, faster for simple text extraction.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (isTesseract) {
      ImGui::Text("Tesseract is enabled.");
      ImGui::TextWrapped("Note: Text orientation can be set using the buttons in the Image section.");
      
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
    } else {
      ImGui::Text("AI OCR is enabled.");
      ImGui::TextWrapped("The selected Text AI provider will be used for OCR.");
    }
  }

} // namespace Image2Card::UI
