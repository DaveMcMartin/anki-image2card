#include "config/ConfigManager.h"

#include <fstream>
#include <iostream>

#include "core/Logger.h"

namespace Image2Card::Config
{

  ConfigManager::ConfigManager(std::string configPath)
      : m_ConfigPath(std::move(configPath))
  {
    Load();
  }

  void ConfigManager::Load()
  {
    std::ifstream file(m_ConfigPath);
    if (!file.is_open()) {
      return;
    }

    try {
      nlohmann::json j;
      file >> j;

      if (j.contains("anki_connect_url"))
        m_Config.AnkiConnectUrl = j["anki_connect_url"];
      if (j.contains("anki_decks"))
        m_Config.AnkiDecks = j["anki_decks"].get<std::vector<std::string>>();
      if (j.contains("anki_note_types"))
        m_Config.AnkiNoteTypes = j["anki_note_types"].get<std::vector<std::string>>();

      if (j.contains("selected_language"))
        m_Config.SelectedLanguage = j["selected_language"];

      if (j.contains("selected_vision_model"))
        m_Config.SelectedVisionModel = j["selected_vision_model"];
      if (j.contains("selected_analysis_model"))
        m_Config.SelectedAnalysisModel = j["selected_analysis_model"];
      if (j.contains("selected_voice_model"))
        m_Config.SelectedVoiceModel = j["selected_voice_model"];

      if (j.contains("text_api_key"))
        m_Config.TextApiKey = j["text_api_key"];
      if (j.contains("text_available_models"))
        m_Config.TextAvailableModels = j["text_available_models"].get<std::vector<std::string>>();

      if (j.contains("google_api_key"))
        m_Config.GoogleApiKey = j["google_api_key"];
      if (j.contains("google_available_models"))
        m_Config.GoogleAvailableModels = j["google_available_models"].get<std::vector<std::string>>();

      // Migration: Convert old google_vision_model/google_sentence_model to new format
      if (j.contains("google_vision_model") && m_Config.SelectedVisionModel.empty()) {
        m_Config.SelectedVisionModel = "Google/" + j["google_vision_model"].get<std::string>();
      }
      if (j.contains("google_sentence_model") && m_Config.SelectedAnalysisModel.empty()) {
        m_Config.SelectedAnalysisModel = "Google/" + j["google_sentence_model"].get<std::string>();
      }
      if (j.contains("google_model") && m_Config.SelectedVisionModel.empty()) {
        m_Config.SelectedVisionModel = "Google/" + j["google_model"].get<std::string>();
      }

      if (j.contains("elevenlabs_api_key"))
        m_Config.ElevenLabsApiKey = j["elevenlabs_api_key"];
      if (j.contains("elevenlabs_voice_id"))
        m_Config.ElevenLabsVoiceId = j["elevenlabs_voice_id"];
      if (j.contains("elevenlabs_available_voices")) {
        m_Config.ElevenLabsAvailableVoices.clear();
        for (const auto& item : j["elevenlabs_available_voices"]) {
          if (item.is_array() && item.size() == 2) {
            m_Config.ElevenLabsAvailableVoices.push_back({item[0], item[1]});
          }
        }
      }

      if (j.contains("minimax_api_key"))
        m_Config.MiniMaxApiKey = j["minimax_api_key"];
      if (j.contains("minimax_voice_id"))
        m_Config.MiniMaxVoiceId = j["minimax_voice_id"];
      if (j.contains("minimax_model"))
        m_Config.MiniMaxModel = j["minimax_model"];
      if (j.contains("minimax_available_voices")) {
        m_Config.MiniMaxAvailableVoices.clear();
        for (const auto& item : j["minimax_available_voices"]) {
          if (item.is_array() && item.size() == 2) {
            m_Config.MiniMaxAvailableVoices.push_back({item[0], item[1]});
          }
        }
      }

      if (j.contains("audio_api_key") && m_Config.ElevenLabsApiKey.empty()) {
        m_Config.ElevenLabsApiKey = j["audio_api_key"];
      }
      if (j.contains("audio_voice_id") && m_Config.ElevenLabsVoiceId.empty()) {
        m_Config.ElevenLabsVoiceId = j["audio_voice_id"];
      }
      if (j.contains("audio_available_voices") && m_Config.ElevenLabsAvailableVoices.empty()) {
        for (const auto& item : j["audio_available_voices"]) {
          if (item.is_array() && item.size() == 2) {
            m_Config.ElevenLabsAvailableVoices.push_back({item[0], item[1]});
          }
        }
      }
      if (j.contains("audio_format")) {
        m_Config.AudioFormat = j["audio_format"];
      }

      if (j.contains("ocr_method"))
        m_Config.OCRMethod = j["ocr_method"];
      if (j.contains("tesseract_orientation"))
        m_Config.TesseractOrientation = j["tesseract_orientation"];

      if (j.contains("audio_provider"))
        m_Config.AudioProvider = j["audio_provider"];

      if (j.contains("deepl_api_key"))
        m_Config.DeepLApiKey = j["deepl_api_key"];
      if (j.contains("deepl_use_free_api"))
        m_Config.DeepLUseFreeAPI = j["deepl_use_free_api"];
      if (j.contains("deepl_source_lang"))
        m_Config.DeepLSourceLang = j["deepl_source_lang"];
      if (j.contains("deepl_target_lang"))
        m_Config.DeepLTargetLang = j["deepl_target_lang"];

      if (j.contains("selected_word_dictionary"))
        m_Config.SelectedWordDictionary = j["selected_word_dictionary"];
      if (j.contains("selected_translator"))
        m_Config.SelectedTranslator = j["selected_translator"];

      if (j.contains("last_note_type"))
        m_Config.LastNoteType = j["last_note_type"];
      if (j.contains("last_deck"))
        m_Config.LastDeck = j["last_deck"];

      if (j.contains("field_mappings")) {
        m_Config.FieldMappings.clear();
        for (auto& [noteType, fields] : j["field_mappings"].items()) {
          for (auto& [fieldName, settings] : fields.items()) {
            if (settings.is_array() && settings.size() == 2) {
              m_Config.FieldMappings[noteType][fieldName] = {settings[0], settings[1]};
            }
          }
        }
      }
    } catch (const std::exception& e) {
      AF_ERROR("Error loading config: {}", e.what());
    }
  }

  void ConfigManager::Save()
  {
    nlohmann::json j;

    j["anki_connect_url"] = m_Config.AnkiConnectUrl;
    j["anki_decks"] = m_Config.AnkiDecks;
    j["anki_note_types"] = m_Config.AnkiNoteTypes;

    j["selected_language"] = m_Config.SelectedLanguage;

    j["selected_vision_model"] = m_Config.SelectedVisionModel;
    j["selected_analysis_model"] = m_Config.SelectedAnalysisModel;
    j["selected_voice_model"] = m_Config.SelectedVoiceModel;

    j["text_api_key"] = m_Config.TextApiKey;
    j["text_available_models"] = m_Config.TextAvailableModels;

    j["google_api_key"] = m_Config.GoogleApiKey;
    j["google_available_models"] = m_Config.GoogleAvailableModels;

    j["elevenlabs_api_key"] = m_Config.ElevenLabsApiKey;
    j["elevenlabs_voice_id"] = m_Config.ElevenLabsVoiceId;

    nlohmann::json elevenLabsVoicesJson = nlohmann::json::array();
    for (const auto& voice : m_Config.ElevenLabsAvailableVoices) {
      elevenLabsVoicesJson.push_back({voice.first, voice.second});
    }
    j["elevenlabs_available_voices"] = elevenLabsVoicesJson;

    j["minimax_api_key"] = m_Config.MiniMaxApiKey;
    j["minimax_voice_id"] = m_Config.MiniMaxVoiceId;
    j["minimax_model"] = m_Config.MiniMaxModel;

    nlohmann::json minimaxVoicesJson = nlohmann::json::array();
    for (const auto& voice : m_Config.MiniMaxAvailableVoices) {
      minimaxVoicesJson.push_back({voice.first, voice.second});
    }
    j["minimax_available_voices"] = minimaxVoicesJson;

    j["ocr_method"] = m_Config.OCRMethod;
    j["tesseract_orientation"] = m_Config.TesseractOrientation;

    j["audio_provider"] = m_Config.AudioProvider;
    j["audio_format"] = m_Config.AudioFormat;

    j["deepl_api_key"] = m_Config.DeepLApiKey;
    j["deepl_use_free_api"] = m_Config.DeepLUseFreeAPI;
    j["deepl_source_lang"] = m_Config.DeepLSourceLang;
    j["deepl_target_lang"] = m_Config.DeepLTargetLang;

    j["selected_word_dictionary"] = m_Config.SelectedWordDictionary;
    j["selected_translator"] = m_Config.SelectedTranslator;

    j["last_note_type"] = m_Config.LastNoteType;
    j["last_deck"] = m_Config.LastDeck;

    nlohmann::json mappingsJson;
    for (const auto& [noteType, fields] : m_Config.FieldMappings) {
      for (const auto& [fieldName, settings] : fields) {
        mappingsJson[noteType][fieldName] = {settings.first, settings.second};
      }
    }
    j["field_mappings"] = mappingsJson;

    std::ofstream file(m_ConfigPath);
    if (file.is_open()) {
      file << j.dump(4);
    } else {
      AF_ERROR("Error saving config to {}", m_ConfigPath);
    }
  }

  AppConfig& ConfigManager::GetConfig()
  {
    return m_Config;
  }

} // namespace Image2Card::Config
