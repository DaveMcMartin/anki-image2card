#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

namespace Image2Card::Config
{

  struct AppConfig
  {
    std::string AnkiConnectUrl = "http://localhost:8765";
    std::vector<std::string> AnkiDecks;
    std::vector<std::string> AnkiNoteTypes;

    std::string SelectedLanguage = "JP";

    // Global Model Selection
    std::string SelectedVisionModel;
    std::string SelectedAnalysisModel;
    std::string SelectedVoiceModel;

    // Dictionary and Translation Selection
    std::string SelectedWordDictionary = "JMDict";
    std::string SelectedTranslator = "google_translate";

    std::string TextApiKey;
    std::vector<std::string> TextAvailableModels;

    std::string GoogleApiKey;
    std::vector<std::string> GoogleAvailableModels;

    std::string AudioApiKey;
    std::string AudioVoiceId;
    std::vector<std::pair<std::string, std::string>> AudioAvailableVoices;

    // OCR Configuration
    std::string OCRMethod = "Tesseract";             // "AI" or "Tesseract"
    std::string TesseractOrientation = "horizontal"; // "horizontal" or "vertical"

    // DeepL Translation Configuration
    std::string DeepLApiKey;
    bool DeepLUseFreeAPI = true;
    std::string DeepLSourceLang = "JA";
    std::string DeepLTargetLang = "EN";

    // Audio Configuration
    std::string AudioProvider = "elevenlabs"; // "elevenlabs" or "native"
    std::string AudioFormat = "mp3";          // "mp3" or "opus"

    // UI State
    std::string LastNoteType;
    std::string LastDeck;
    std::map<std::string, std::map<std::string, std::pair<bool, int>>> FieldMappings;
  };

  class ConfigManager
  {
public:

    explicit ConfigManager(std::string configPath = "config.json");
    ~ConfigManager() = default;

    void Load();
    void Save();

    AppConfig& GetConfig();

private:

    std::string m_ConfigPath;
    AppConfig m_Config;
  };

} // namespace Image2Card::Config
