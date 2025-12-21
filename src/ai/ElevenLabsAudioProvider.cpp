#include "ai/ElevenLabsAudioProvider.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <exception>
#include <httplib.h>
#include <iostream>
#include <thread>

#include "core/Logger.h"

namespace Image2Card::AI
{

  ElevenLabsAudioProvider::ElevenLabsAudioProvider() {}

  ElevenLabsAudioProvider::~ElevenLabsAudioProvider() {}

  bool ElevenLabsAudioProvider::RenderConfigurationUI()
  {
    bool changed = false;

    if (ImGui::InputText("API Key", &m_ApiKey, ImGuiInputTextFlags_Password)) {
      changed = true;
    }

    if (ImGui::Button("Load Voices")) {
      std::thread([this]() { LoadRemoteVoices(); }).detach();
      changed = true;
    }

    if (m_IsLoadingVoices) {
      ImGui::SameLine();
      ImGui::Text("Loading...");
    } else if (!m_StatusMessage.empty()) {
      ImGui::SameLine();
      ImGui::Text("%s", m_StatusMessage.c_str());
    }

    if (RenderVoiceSelector("Voice", &m_VoiceId)) {
      changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Audio Format:");
    static const char* formats[] = {"MP3", "Opus"};
    static const char* formatValues[] = {"mp3", "opus"};
    
    int currentFormatIdx = 0;
    if (m_AudioFormat == "opus") {
      currentFormatIdx = 1;
    }

    if (ImGui::Combo("##audio_format", &currentFormatIdx, formats, IM_ARRAYSIZE(formats))) {
      m_AudioFormat = formatValues[currentFormatIdx];
      changed = true;
    }

    return changed;
  }

  bool ElevenLabsAudioProvider::RenderVoiceSelector(const char* label, std::string* selectedVoiceId)
  {
    bool changed = false;

    if (!selectedVoiceId)
      return false;

    std::string currentVoiceName = *selectedVoiceId;

    // Find the voice name for the current ID
    for (const auto& voice : m_AvailableVoices) {
      if (voice.Id == *selectedVoiceId) {
        currentVoiceName = voice.Name;
        break;
      }
    }

    if (ImGui::BeginCombo(label, currentVoiceName.c_str())) {
      for (const auto& voice : m_AvailableVoices) {
        bool is_selected = (*selectedVoiceId == voice.Id);
        if (ImGui::Selectable(voice.Name.c_str(), is_selected)) {
          *selectedVoiceId = voice.Id;
          changed = true;
        }
        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    return changed;
  }

  void ElevenLabsAudioProvider::LoadConfig(const nlohmann::json& json)
  {
    if (json.contains("api_key"))
      m_ApiKey = json["api_key"];
    if (json.contains("voice_id"))
      m_VoiceId = json["voice_id"];
    if (json.contains("audio_format"))
      m_AudioFormat = json["audio_format"];
    if (json.contains("available_voices")) {
      m_AvailableVoices.clear();
      for (const auto& item : json["available_voices"]) {
        if (item.is_array() && item.size() == 2) {
          m_AvailableVoices.push_back({item[0], item[1]});
        }
      }
    }
  }

  nlohmann::json ElevenLabsAudioProvider::SaveConfig() const
  {
    nlohmann::json voicesJson = nlohmann::json::array();
    for (const auto& voice : m_AvailableVoices) {
      voicesJson.push_back({voice.Id, voice.Name});
    }

    return {{"api_key", m_ApiKey}, {"voice_id", m_VoiceId}, {"audio_format", m_AudioFormat}, {"available_voices", voicesJson}};
  }

  void ElevenLabsAudioProvider::LoadRemoteVoices()
  {
    if (m_ApiKey.empty()) {
      m_StatusMessage = "Error: API Key required.";
      return;
    }

    m_IsLoadingVoices = true;
    m_StatusMessage = "";

    try {
      httplib::Client cli("https://api.elevenlabs.io");
      cli.set_connection_timeout(120);
      cli.set_read_timeout(120);

      httplib::Headers headers = {{"xi-api-key", m_ApiKey}};

      auto res = cli.Get("/v1/voices", headers);

      if (res && res->status == 200) {
        auto json = nlohmann::json::parse(res->body);
        m_AvailableVoices.clear();
        if (json.contains("voices") && json["voices"].is_array()) {
          for (const auto& item : json["voices"]) {
            if (item.contains("voice_id") && item.contains("name")) {
              m_AvailableVoices.push_back({item["voice_id"], item["name"]});
            }
          }
        }

        std::sort(m_AvailableVoices.begin(),
                  m_AvailableVoices.end(),
                  [](const ElevenLabsVoice& a, const ElevenLabsVoice& b) { return a.Name < b.Name; });

        m_StatusMessage = "Voices loaded.";
      } else {
        m_StatusMessage = "Error loading voices: " + std::to_string(res ? res->status : 0);
      }
    } catch (const std::exception& e) {
      m_StatusMessage = std::string("Exception: ") + e.what();
    }

    m_IsLoadingVoices = false;
  }

  std::vector<unsigned char> ElevenLabsAudioProvider::GenerateAudio(const std::string& text,
                                                                    const std::string& voiceId,
                                                                    const std::string& languageCode,
                                                                    const std::string& format)
  {
    std::string targetVoiceId = voiceId.empty() ? m_VoiceId : voiceId;
    std::string audioFormat = format.empty() ? m_AudioFormat : format;

    if (m_ApiKey.empty() || targetVoiceId.empty()) {
      AF_ERROR("ElevenLabsAudioProvider Error: API Key or Voice ID is missing.");
      return {};
    }

    try {
      httplib::Client cli("https://api.elevenlabs.io");
      cli.set_connection_timeout(120);
      cli.set_read_timeout(120);

      // Set correct Accept header based on format
      std::string acceptHeader = (audioFormat == "opus") ? "audio/opus" : "audio/mpeg";
      httplib::Headers headers = {{"xi-api-key", m_ApiKey}, {"Accept", acceptHeader}};

      nlohmann::json payload = {{"text", text},
                                {"model_id", "eleven_v3"},
                                {"voice_settings", {{"stability", 0.5}, {"similarity_boost", 0.75}}}};

      // Add language code if provided
      if (!languageCode.empty()) {
        payload["language_code"] = languageCode;
      }

      // Add output format if not default MP3
      if (audioFormat == "opus") {
        payload["output_format"] = "opus_64";
      }

      std::string endpoint = "/v1/text-to-speech/" + targetVoiceId;

      auto res = cli.Post(endpoint, headers, payload.dump(), "application/json");

      if (res && res->status == 200) {
        AF_INFO("Generated audio in {} format, size: {} bytes", audioFormat, res->body.size());
        return std::vector<unsigned char>(res->body.begin(), res->body.end());
      } else {
        AF_ERROR("ElevenLabsAudioProvider HTTP Error: {}", (res ? res->status : 0));
        if (res)
          AF_ERROR("Response: {}", res->body);
        return {};
      }
    } catch (const std::exception& e) {
      AF_ERROR("ElevenLabsAudioProvider Exception: {}", e.what());
      return {};
    }
  }

} // namespace Image2Card::AI
