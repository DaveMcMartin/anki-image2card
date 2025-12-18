#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <nlohmann/json.hpp>

namespace Image2Card::Config
{

struct AppConfig
{
    std::string AnkiConnectUrl = "http://localhost:8765";
    std::vector<std::string> AnkiDecks;
    std::vector<std::string> AnkiNoteTypes;

    std::string SelectedLanguage = "JP";

    std::string TextProvider = "xAI";
    std::string TextApiKey;
    std::string TextVisionModel = "grok-2-vision-1212";
    std::string TextSentenceModel = "grok-2-1212";
    std::vector<std::string> TextAvailableModels;

    std::string GoogleApiKey;
    std::string GoogleModel = "gemini-2.0-flash";
    std::vector<std::string> GoogleAvailableModels;

    std::string AudioProvider = "ElevenLabs";
    std::string AudioApiKey;
    std::string AudioVoiceId;
    std::vector<std::pair<std::string, std::string>> AudioAvailableVoices;

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
