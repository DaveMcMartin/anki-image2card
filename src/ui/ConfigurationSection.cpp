#include "ui/ConfigurationSection.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <thread>

#include "ai/IAudioAIProvider.h"
#include "ai/ITextAIProvider.h"
#include "api/AnkiConnectClient.h"
#include "config/ConfigManager.h"
#include "language/ILanguage.h"
#include "language/services/ILanguageService.h"

namespace Image2Card::UI
{

  ConfigurationSection::ConfigurationSection(
      API::AnkiConnectClient* ankiConnectClient,
      Config::ConfigManager* configManager,
      std::vector<std::shared_ptr<AI::ITextAIProvider>>* textAIProviders,
      AI::IAudioAIProvider* audioAIProvider,
      std::vector<std::unique_ptr<Language::Services::ILanguageService>>* languageServices,
      std::vector<std::unique_ptr<Language::ILanguage>>* languages,
      Language::ILanguage** activeLanguage)
      : m_AnkiConnectClient(ankiConnectClient)
      , m_ConfigManager(configManager)
      , m_TextAIProviders(textAIProviders)
      , m_AudioAIProvider(audioAIProvider)
      , m_LanguageServices(languageServices)
      , m_Languages(languages)
      , m_ActiveLanguage(activeLanguage)
  {}

  ConfigurationSection::~ConfigurationSection() {}

  void ConfigurationSection::Render()
  {
    RenderAnkiConnectTab();
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

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Text("Default Note Type and Deck");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("These will be used as defaults when creating cards.");
    ImGui::Spacing();

    ImGui::Text("Default Note Type");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##DefaultNoteType", config.LastNoteType.c_str())) {
      for (const auto& noteType : config.AnkiNoteTypes) {
        bool isSelected = (config.LastNoteType == noteType);
        if (ImGui::Selectable(noteType.c_str(), isSelected)) {
          config.LastNoteType = noteType;
          m_ConfigManager->Save();
          if (m_OnNoteTypeOrDeckChangedCallback) {
            m_OnNoteTypeOrDeckChangedCallback();
          }
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::Spacing();

    ImGui::Text("Default Deck");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##DefaultDeck", config.LastDeck.c_str())) {
      for (const auto& deck : config.AnkiDecks) {
        bool isSelected = (config.LastDeck == deck);
        if (ImGui::Selectable(deck.c_str(), isSelected)) {
          config.LastDeck = deck;
          m_ConfigManager->Save();
          if (m_OnNoteTypeOrDeckChangedCallback) {
            m_OnNoteTypeOrDeckChangedCallback();
          }
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
  }

  void ConfigurationSection::RenderOCRTab()
  {
    if (!m_ConfigManager)
      return;

    auto& config = m_ConfigManager->GetConfig();

    ImGui::Spacing();
    ImGui::Text("OCR Method");
    ImGui::Separator();
    ImGui::Spacing();

    bool isTesseract = (config.OCRMethod == "Tesseract");
    bool isAI = (config.OCRMethod == "AI");

    if (ImGui::RadioButton("Tesseract (Local, Offline)", isTesseract)) {
      config.OCRMethod = "Tesseract";
      m_ConfigManager->Save();
    }

    if (ImGui::RadioButton("AI (Cloud-based, More Accurate)", isAI)) {
      config.OCRMethod = "AI";
      m_ConfigManager->Save();
    }

    ImGui::Spacing();

    if (isTesseract) {
      ImGui::Text("Text Orientation");
      ImGui::Spacing();

      bool isHorizontal = (config.TesseractOrientation == "horizontal");
      bool isVertical = (config.TesseractOrientation == "vertical");

      if (ImGui::RadioButton("Horizontal", isHorizontal)) {
        config.TesseractOrientation = "horizontal";
        m_ConfigManager->Save();
      }
      ImGui::SameLine();
      if (ImGui::RadioButton("Vertical", isVertical)) {
        config.TesseractOrientation = "vertical";
        m_ConfigManager->Save();
      }
    }

    if (isAI) {
      ImGui::Spacing();
      ImGui::Text("Vision Model");
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::Text("Select AI Model for OCR");
      ImGui::SetNextItemWidth(-1);

      std::string currentModel = config.SelectedVisionModel;
      if (currentModel.empty() && m_TextAIProviders && !m_TextAIProviders->empty()) {
        currentModel = (*m_TextAIProviders)[0]->GetId() + "/default";
      }

      if (ImGui::BeginCombo("##VisionModel", currentModel.c_str())) {
        if (m_TextAIProviders) {
          for (auto& provider : *m_TextAIProviders) {
            std::string providerId = provider->GetId();
            std::string providerName = provider->GetName();

            ImGui::PushID(providerId.c_str());
            if (ImGui::TreeNode(providerName.c_str())) {
              ImGui::PopID();

              nlohmann::json providerConfig = provider->SaveConfig();
              if (providerConfig.contains("available_models") && providerConfig["available_models"].is_array()) {
                for (const auto& model : providerConfig["available_models"]) {
                  if (model.is_string()) {
                    std::string modelName = model.get<std::string>();
                    std::string fullModelName = providerId + "/" + modelName;
                    bool isSelected = (currentModel == fullModelName);

                    if (ImGui::Selectable(modelName.c_str(), isSelected)) {
                      config.SelectedVisionModel = fullModelName;
                      m_ConfigManager->Save();
                    }
                    if (isSelected) {
                      ImGui::SetItemDefaultFocus();
                    }
                  }
                }
              }
              ImGui::TreePop();
            } else {
              ImGui::PopID();
            }
          }
        }
        ImGui::EndCombo();
      }
    }
  }

  void ConfigurationSection::RenderDictionaryTab()
  {
    if (!m_ConfigManager)
      return;

    auto& config = m_ConfigManager->GetConfig();

    ImGui::Spacing();
    ImGui::Text("Target Word Definition");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("Select which dictionary to use for looking up target word definitions.");
    ImGui::Spacing();

    ImGui::Text("Dictionary Source");
    ImGui::SetNextItemWidth(-1);

    std::string currentWordDict = config.SelectedWordDictionary.empty() ? "JMDict" : config.SelectedWordDictionary;
    if (ImGui::BeginCombo("##WordDictionary", currentWordDict.c_str())) {
      const char* wordDictOptions[] = {"JMDict", "DeepL", "Google Translate", "Gemini", "xAI"};
      for (const char* option : wordDictOptions) {
        bool isSelected = (currentWordDict == option);
        if (ImGui::Selectable(option, isSelected)) {
          config.SelectedWordDictionary = option;
          m_ConfigManager->Save();
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Text("Sentence Translation");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("Select which service to use for translating sentences.");
    ImGui::Spacing();

    ImGui::Text("Translation Service");
    ImGui::SetNextItemWidth(-1);

    std::string currentTranslatorId = config.SelectedTranslator.empty() ? "none" : config.SelectedTranslator;
    std::string currentTranslatorName = "None";
    if (m_LanguageServices) {
      for (const auto& service : *m_LanguageServices) {
        if (service->GetType() == "translator" && service->GetId() == currentTranslatorId) {
          currentTranslatorName = service->GetName();
          break;
        }
      }
    }

    if (ImGui::BeginCombo("##SentenceTranslator", currentTranslatorName.c_str())) {
      if (m_LanguageServices) {
        for (const auto& service : *m_LanguageServices) {
          if (service->GetType() == "translator") {
            std::string serviceName = service->GetName();
            std::string serviceId = service->GetId();
            bool isSelected = (currentTranslatorId == serviceId);

            if (ImGui::Selectable(serviceName.c_str(), isSelected)) {
              config.SelectedTranslator = serviceId;
              m_ConfigManager->Save();
              if (m_OnTranslatorChangedCallback) {
                m_OnTranslatorChangedCallback(serviceId);
              }
            }
            if (isSelected) {
              ImGui::SetItemDefaultFocus();
            }
          }
        }
      }
      ImGui::EndCombo();
    }
  }

  void ConfigurationSection::RenderConfigurationTab()
  {
    if (!m_ConfigManager)
      return;

    auto& config = m_ConfigManager->GetConfig();

    ImGui::Spacing();
    ImGui::Text("Audio Configuration");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Audio Provider");
    ImGui::SetNextItemWidth(-1);

    // Simple combo for now
    std::string currentProvider = config.AudioProvider;
    if (currentProvider.empty())
      currentProvider = "elevenlabs";

    if (ImGui::BeginCombo("##AudioProvider", (currentProvider == "elevenlabs" ? "ElevenLabs" : "Native (OS Default)")))
    {
      if (ImGui::Selectable("ElevenLabs", currentProvider == "elevenlabs")) {
        config.AudioProvider = "elevenlabs";
        m_ConfigManager->Save();
        if (m_OnAudioProviderChangedCallback)
          m_OnAudioProviderChangedCallback("elevenlabs");
      }
      if (ImGui::Selectable("Native (OS Default)", currentProvider == "native")) {
        config.AudioProvider = "native";
        m_ConfigManager->Save();
        if (m_OnAudioProviderChangedCallback)
          m_OnAudioProviderChangedCallback("native");
      }
      ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (m_AudioAIProvider) {
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

      ImGui::Spacing();
      ImGui::Text("Voice Model");
      ImGui::SetNextItemWidth(-1);

      std::string currentVoiceLabel = config.SelectedVoiceModel;
      for (const auto& voice : config.AudioAvailableVoices) {
        if ("ElevenLabs/" + voice.first == config.SelectedVoiceModel) {
          currentVoiceLabel = "ElevenLabs/" + voice.second;
          break;
        }
      }

      if (ImGui::BeginCombo("##VoiceModel", currentVoiceLabel.c_str())) {
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

      ImGui::Spacing();
      ImGui::Text("Audio Format");
      ImGui::SetNextItemWidth(-1);

      if (ImGui::BeginCombo("##AudioFormat", config.AudioFormat.c_str())) {
        const char* formats[] = {"mp3", "opus"};
        for (const char* format : formats) {
          bool isSelected = (config.AudioFormat == format);
          if (ImGui::Selectable(format, isSelected)) {
            config.AudioFormat = format;
            m_ConfigManager->Save();
          }
          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Text("DeepL Translation");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("API Key");
    if (ImGui::InputText("##DeepLApiKey", &config.DeepLApiKey, ImGuiInputTextFlags_Password)) {
      m_ConfigManager->Save();
    }

    ImGui::Spacing();

    bool useFreeApi = config.DeepLUseFreeAPI;
    if (ImGui::Checkbox("Use Free API", &useFreeApi)) {
      config.DeepLUseFreeAPI = useFreeApi;
      m_ConfigManager->Save();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Text("AI Providers");
    ImGui::Separator();
    ImGui::Spacing();

    if (m_TextAIProviders) {
      for (auto& provider : *m_TextAIProviders) {
        ImGui::PushID(provider->GetId().c_str());
        ImGui::Spacing();
        ImGui::Text("%s", provider->GetName().c_str());
        ImGui::Separator();
        ImGui::Spacing();

        if (provider->RenderConfigurationUI()) {
          auto j = provider->SaveConfig();

          if (provider->GetId() == "google") {
            if (j.contains("api_key"))
              config.GoogleApiKey = j["api_key"];
            if (j.contains("available_models"))
              config.GoogleAvailableModels = j["available_models"].get<std::vector<std::string>>();
          } else if (provider->GetId() == "xai") {
            if (j.contains("api_key"))
              config.TextApiKey = j["api_key"];
            if (j.contains("available_models"))
              config.TextAvailableModels = j["available_models"].get<std::vector<std::string>>();
          }

          m_ConfigManager->Save();
        }
        ImGui::PopID();
        ImGui::Spacing();
      }
    }
  }

} // namespace Image2Card::UI
