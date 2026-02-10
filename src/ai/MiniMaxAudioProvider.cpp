#include "ai/MiniMaxAudioProvider.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <exception>
#include <httplib.h>
#include <iostream>
#include <miniaudio.h>
#include <sstream>
#include <thread>

#include "core/Logger.h"

namespace Image2Card::AI
{

  MiniMaxAudioProvider::MiniMaxAudioProvider() {}

  MiniMaxAudioProvider::~MiniMaxAudioProvider() {}

  bool MiniMaxAudioProvider::RenderConfigurationUI()
  {
    bool changed = false;

    if (ImGui::InputText("API Key", &m_ApiKey, ImGuiInputTextFlags_Password)) {
      changed = true;
    }

    if (m_VoicesUpdated.exchange(false)) {
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

    ImGui::Spacing();

    const char* models[] = {
        "speech-2.8-hd", "speech-2.8-turbo", "speech-2.6-hd", "speech-2.6-turbo", "speech-02-hd", "speech-02-turbo"};
    int currentModelIdx = 0;
    for (int i = 0; i < 6; i++) {
      if (m_Model == models[i]) {
        currentModelIdx = i;
        break;
      }
    }

    if (ImGui::Combo("Model", &currentModelIdx, models, 6)) {
      m_Model = models[currentModelIdx];
      changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();

    return changed;
  }

  bool MiniMaxAudioProvider::RenderVoiceSelector(const char* label, std::string* selectedVoiceId)
  {
    bool changed = false;

    if (!selectedVoiceId)
      return false;

    std::string currentVoiceName = *selectedVoiceId;

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

  void MiniMaxAudioProvider::LoadConfig(const nlohmann::json& json)
  {
    if (json.contains("api_key"))
      m_ApiKey = json["api_key"];
    if (json.contains("voice_id"))
      m_VoiceId = json["voice_id"];
    if (json.contains("model"))
      m_Model = json["model"];
    if (json.contains("available_voices")) {
      m_AvailableVoices.clear();
      for (const auto& item : json["available_voices"]) {
        if (item.is_array() && item.size() >= 2) {
          MiniMaxVoice voice;
          voice.Id = item[0];
          voice.Name = item[1];
          if (item.size() >= 3)
            voice.Type = item[2];
          m_AvailableVoices.push_back(voice);
        }
      }
    }
  }

  nlohmann::json MiniMaxAudioProvider::SaveConfig() const
  {
    nlohmann::json voicesJson = nlohmann::json::array();
    for (const auto& voice : m_AvailableVoices) {
      voicesJson.push_back({voice.Id, voice.Name});
    }

    return {{"api_key", m_ApiKey}, {"voice_id", m_VoiceId}, {"model", m_Model}, {"available_voices", voicesJson}};
  }

  void MiniMaxAudioProvider::LoadRemoteVoices()
  {
    if (m_ApiKey.empty()) {
      m_StatusMessage = "Error: API Key required.";
      return;
    }

    m_IsLoadingVoices = true;
    m_StatusMessage = "";

    try {
      httplib::Client cli("https://api.minimax.io");
      cli.set_connection_timeout(120);
      cli.set_read_timeout(120);

      httplib::Headers headers = {{"Authorization", "Bearer " + m_ApiKey}, {"Content-Type", "application/json"}};

      nlohmann::json requestBody = {{"voice_type", "all"}};

      auto res = cli.Post("/v1/get_voice", headers, requestBody.dump(), "application/json");

      if (res && res->status == 200) {
        auto json = nlohmann::json::parse(res->body);
        m_AvailableVoices.clear();

        AF_INFO("MiniMax get_voice response: {}", json.dump());

        if (json.contains("base_resp") && json["base_resp"].contains("status_code") &&
            json["base_resp"]["status_code"] == 0)
        {
          if (json.contains("system_voice") && json["system_voice"].is_array()) {
            AF_INFO("Found {} system voices", json["system_voice"].size());
            for (const auto& item : json["system_voice"]) {
              if (item.contains("voice_id") && item.contains("voice_name")) {
                std::string voiceId = item["voice_id"];
                if (voiceId.find("Japanese_") == 0) {
                  MiniMaxVoice voice;
                  voice.Id = voiceId;
                  voice.Name = item["voice_name"];
                  voice.Type = "system";
                  m_AvailableVoices.push_back(voice);
                  AF_INFO("Added Japanese voice: {} ({})", voice.Name, voice.Id);
                }
              }
            }
          }

          if (json.contains("voice_cloning") && json["voice_cloning"].is_array()) {
            for (const auto& item : json["voice_cloning"]) {
              if (item.contains("voice_id")) {
                MiniMaxVoice voice;
                voice.Id = item["voice_id"];
                voice.Name = item["voice_id"];
                voice.Type = "cloning";
                m_AvailableVoices.push_back(voice);
              }
            }
          }

          if (json.contains("voice_generation") && json["voice_generation"].is_array()) {
            for (const auto& item : json["voice_generation"]) {
              if (item.contains("voice_id")) {
                MiniMaxVoice voice;
                voice.Id = item["voice_id"];
                voice.Name = item["voice_id"];
                voice.Type = "generation";
                m_AvailableVoices.push_back(voice);
              }
            }
          }

          std::sort(
              m_AvailableVoices.begin(), m_AvailableVoices.end(), [](const MiniMaxVoice& a, const MiniMaxVoice& b) {
                if (a.Type != b.Type)
                  return a.Type < b.Type;
                return a.Name < b.Name;
              });

          AF_INFO("Total voices loaded: {}", m_AvailableVoices.size());
          m_StatusMessage = "Voices loaded.";
          m_VoicesUpdated.store(true);
        } else {
          auto statusMsg = json.contains("base_resp") && json["base_resp"].contains("status_msg")
                               ? json["base_resp"]["status_msg"].get<std::string>()
                               : "Unknown error";
          m_StatusMessage = "Error: " + statusMsg;
          AF_ERROR("MiniMax API error: {}", statusMsg);
        }
      } else {
        m_StatusMessage = "Error loading voices: " + std::to_string(res ? res->status : 0);
        AF_ERROR("MiniMax HTTP error: {}", (res ? res->status : 0));
        if (res)
          AF_ERROR("Response body: {}", res->body);
      }
    } catch (const std::exception& e) {
      m_StatusMessage = std::string("Exception: ") + e.what();
    }

    m_IsLoadingVoices = false;
  }

  std::vector<unsigned char> MiniMaxAudioProvider::ConvertMp3ToOgg(const std::vector<unsigned char>& mp3Data)
  {
    static auto writeCallback =
        [](ma_encoder* pEncoder, const void* pBufferIn, size_t bytesToWrite, size_t* pBytesWritten) -> ma_result {
      std::vector<unsigned char>* pOggData = static_cast<std::vector<unsigned char>*>(pEncoder->pUserData);
      const unsigned char* pBytes = static_cast<const unsigned char*>(pBufferIn);
      pOggData->insert(pOggData->end(), pBytes, pBytes + bytesToWrite);
      *pBytesWritten = bytesToWrite;
      return MA_SUCCESS;
    };

    static auto seekCallback = [](ma_encoder* pEncoder, ma_int64 offset, ma_seek_origin origin) -> ma_result {
      return MA_SUCCESS;
    };

    ma_decoder decoder;
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_s16, 1, 32000);

    ma_result result = ma_decoder_init_memory(mp3Data.data(), mp3Data.size(), &decoderConfig, &decoder);
    if (result != MA_SUCCESS) {
      AF_ERROR("Failed to initialize decoder for mp3 to ogg conversion");
      return {};
    }

    ma_uint64 totalFrames = 0;
    result = ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    if (result != MA_SUCCESS || totalFrames == 0) {
      AF_WARN("Could not determine mp3 length, using fallback");
      totalFrames = 32000 * 10;
    }

    std::vector<int16_t> pcmData(totalFrames * decoder.outputChannels);
    ma_uint64 framesRead = 0;
    result = ma_decoder_read_pcm_frames(&decoder, pcmData.data(), totalFrames, &framesRead);

    if (result != MA_SUCCESS && result != MA_AT_END) {
      AF_ERROR("Failed to decode mp3 data");
      ma_decoder_uninit(&decoder);
      return {};
    }

    pcmData.resize(framesRead * decoder.outputChannels);
    ma_uint32 sampleRate = decoder.outputSampleRate;
    ma_uint32 channels = decoder.outputChannels;
    ma_decoder_uninit(&decoder);

    std::vector<unsigned char> oggData;
    oggData.reserve(pcmData.size() * sizeof(int16_t));

    ma_encoder encoder;
    encoder.pUserData = &oggData;

    ma_encoder_config encoderConfig =
        ma_encoder_config_init(ma_encoding_format_vorbis, ma_format_s16, channels, sampleRate);

    result = ma_encoder_init(+writeCallback, +seekCallback, &oggData, &encoderConfig, &encoder);
    if (result != MA_SUCCESS) {
      AF_ERROR("Failed to initialize ogg encoder");
      return {};
    }

    ma_uint64 framesWritten = 0;
    result = ma_encoder_write_pcm_frames(&encoder, pcmData.data(), framesRead, &framesWritten);

    if (result != MA_SUCCESS) {
      AF_ERROR("Failed to encode pcm to ogg");
      ma_encoder_uninit(&encoder);
      return {};
    }

    ma_encoder_uninit(&encoder);

    AF_INFO("Converted mp3 ({} bytes) to ogg ({} bytes)", mp3Data.size(), oggData.size());
    return oggData;
  }

  std::vector<unsigned char> MiniMaxAudioProvider::GenerateAudio(const std::string& text,
                                                                 const std::string& voiceId,
                                                                 const std::string& languageCode,
                                                                 const std::string& format)
  {
    std::string targetVoiceId = voiceId.empty() ? m_VoiceId : voiceId;

    if (m_ApiKey.empty() || targetVoiceId.empty()) {
      AF_ERROR("MiniMaxAudioProvider Error: API Key or Voice ID is missing.");
      return {};
    }

    try {
      httplib::Client cli("https://api.minimax.io");
      cli.set_connection_timeout(120);
      cli.set_read_timeout(120);

      httplib::Headers headers = {{"Authorization", "Bearer " + m_ApiKey}, {"Content-Type", "application/json"}};

      nlohmann::json voiceSetting = {{"voice_id", targetVoiceId}, {"speed", 1.0}, {"vol", 1.0}, {"pitch", 0}};

      nlohmann::json audioSetting = {{"sample_rate", 32000}, {"bitrate", 128000}, {"format", "mp3"}, {"channel", 1}};

      nlohmann::json payload = {{"model", m_Model},
                                {"text", text},
                                {"stream", false},
                                {"voice_setting", voiceSetting},
                                {"audio_setting", audioSetting},
                                {"output_format", "hex"}};

      if (!languageCode.empty()) {
        payload["language_boost"] = languageCode;
      }

      auto res = cli.Post("/v1/t2a_v2", headers, payload.dump(), "application/json");

      if (res && res->status == 200) {
        auto json = nlohmann::json::parse(res->body);

        if (json.contains("base_resp") && json["base_resp"].contains("status_code") &&
            json["base_resp"]["status_code"] == 0)
        {
          if (json.contains("data") && json["data"].contains("audio")) {
            std::string hexAudio = json["data"]["audio"];

            std::vector<unsigned char> audioData;
            audioData.reserve(hexAudio.length() / 2);

            for (size_t i = 0; i < hexAudio.length(); i += 2) {
              std::string byteString = hexAudio.substr(i, 2);
              unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
              audioData.push_back(byte);
            }

            AF_INFO("Generated audio with MiniMax model {}, size: {} bytes", m_Model, audioData.size());

            if (format == "opus") {
              AF_INFO("Converting mp3 to ogg for opus format request");
              return ConvertMp3ToOgg(audioData);
            }

            return audioData;
          }
        }

        auto statusMsg = json.contains("base_resp") && json["base_resp"].contains("status_msg")
                             ? json["base_resp"]["status_msg"].get<std::string>()
                             : "Unknown error";
        AF_ERROR("MiniMaxAudioProvider Error: {}", statusMsg);
        return {};
      } else {
        AF_ERROR("MiniMaxAudioProvider HTTP Error: {}", (res ? res->status : 0));
        if (res)
          AF_ERROR("Response: {}", res->body);
        return {};
      }
    } catch (const std::exception& e) {
      AF_ERROR("MiniMaxAudioProvider Exception: {}", e.what());
      return {};
    }
  }

} // namespace Image2Card::AI
