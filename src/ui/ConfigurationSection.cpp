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
                                             AI::IAudioAIProvider* audioAIProvider,
                                             std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                                             Language::ILanguage** activeLanguage)
      : m_AnkiConnectClient(ankiConnectClient)
      , m_ConfigManager(configManager)
      , m_TextAIProviders(textAIProviders)
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
      if (ImGui::BeginTabItem("AI")) {
        RenderAITab();
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

  void ConfigurationSection::RenderAITab()
  {
    if (!m_ConfigManager)
      return;

    auto& config = m_ConfigManager->GetConfig();

    // 1. ElevenLabs (Audio)
    if (m_AudioAIProvider) {
      ImGui::PushID("AudioProvider");
      ImGui::Spacing();
      ImGui::Text("%s", m_AudioAIProvider->GetName().c_str());
      ImGui::Separator();
      ImGui::Spacing();

      if (m_AudioAIProvider->RenderConfigurationUI()) {
        auto j = m_AudioAIProvider->SaveConfig();

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
      ImGui::PopID();
    }

    // 2. Text Providers (Google, xAI)
    if (m_TextAIProviders) {
      // Google
      for (auto& provider : *m_TextAIProviders) {
        if (provider->GetId() == "google") {
          ImGui::PushID("GoogleProvider");
          ImGui::Spacing();
          ImGui::Text("%s", provider->GetName().c_str());
          ImGui::Separator();
          ImGui::Spacing();

          if (provider->RenderConfigurationUI()) {
            auto j = provider->SaveConfig();
            if (j.contains("api_key"))
              config.GoogleApiKey = j["api_key"];

            if (j.contains("available_models"))
              config.GoogleAvailableModels = j["available_models"].get<std::vector<std::string>>();
            m_ConfigManager->Save();
          }
          ImGui::PopID();
        }
      }

      // xAI
      for (auto& provider : *m_TextAIProviders) {
        if (provider->GetId() == "xai") {
          ImGui::PushID("xAIProvider");
          ImGui::Spacing();
          ImGui::Text("%s", provider->GetName().c_str());
          ImGui::Separator();
          ImGui::Spacing();

          if (provider->RenderConfigurationUI()) {
            auto j = provider->SaveConfig();
            if (j.contains("api_key"))
              config.TextApiKey = j["api_key"];

            if (j.contains("available_models"))
              config.TextAvailableModels = j["available_models"].get<std::vector<std::string>>();
            m_ConfigManager->Save();
          }
          ImGui::PopID();
        }
      }
    }

    // 3. Action Configuration
    ImGui::Spacing();
    ImGui::Text("Action Configuration");
    ImGui::Separator();
    ImGui::Spacing();

    // Vision Model
    if (ImGui::BeginCombo("Vision Model", config.SelectedVisionModel.c_str())) {
      // xAI Models
      for (const auto& model : config.TextAvailableModels) {
        std::string label = "xAI/" + model;
        bool isSelected = (config.SelectedVisionModel == label);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
          config.SelectedVisionModel = label;
          m_ConfigManager->Save();
        }
        if (isSelected)
          ImGui::SetItemDefaultFocus();
      }
      // Google Models
      for (const auto& model : config.GoogleAvailableModels) {
        std::string label = "Google/" + model;
        bool isSelected = (config.SelectedVisionModel == label);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
          config.SelectedVisionModel = label;
          m_ConfigManager->Save();
        }
        if (isSelected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // Analysis Model
    if (ImGui::BeginCombo("Analysis Model", config.SelectedAnalysisModel.c_str())) {
      // xAI Models
      for (const auto& model : config.TextAvailableModels) {
        std::string label = "xAI/" + model;
        bool isSelected = (config.SelectedAnalysisModel == label);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
          config.SelectedAnalysisModel = label;
          m_ConfigManager->Save();
        }
        if (isSelected)
          ImGui::SetItemDefaultFocus();
      }
      // Google Models
      for (const auto& model : config.GoogleAvailableModels) {
        std::string label = "Google/" + model;
        bool isSelected = (config.SelectedAnalysisModel == label);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
          config.SelectedAnalysisModel = label;
          m_ConfigManager->Save();
        }
        if (isSelected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // Voice Model
    std::string currentVoiceLabel = config.SelectedVoiceModel;
    for (const auto& voice : config.AudioAvailableVoices) {
      if ("ElevenLabs/" + voice.first == config.SelectedVoiceModel) {
        currentVoiceLabel = "ElevenLabs/" + voice.second;
        break;
      }
    }

    if (ImGui::BeginCombo("Voice Model", currentVoiceLabel.c_str())) {
      for (const auto& voice : config.AudioAvailableVoices) {
        std::string label = "ElevenLabs/" + voice.second;
        std::string value = "ElevenLabs/" + voice.first;
        bool isSelected = (config.SelectedVoiceModel == value);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
          config.SelectedVoiceModel = value;
          config.AudioVoiceId = voice.first;
          m_ConfigManager->Save();
        }
        if (isSelected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // Audio Format
    static const char* formats[] = {"MP3", "Opus"};
    static const char* formatValues[] = {"mp3", "opus"};

    int currentFormatIdx = 0;
    if (config.AudioFormat == "opus") {
      currentFormatIdx = 1;
    }

    if (ImGui::Combo("Audio Format", &currentFormatIdx, formats, IM_ARRAYSIZE(formats))) {
      config.AudioFormat = formatValues[currentFormatIdx];
      m_ConfigManager->Save();
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
