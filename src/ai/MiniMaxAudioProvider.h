#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "ai/IAudioAIProvider.h"

namespace Image2Card::AI
{

  struct MiniMaxVoice
  {
    std::string Id;
    std::string Name;
    std::string Type;
  };

  class MiniMaxAudioProvider : public IAudioAIProvider
  {
public:

    MiniMaxAudioProvider();
    ~MiniMaxAudioProvider() override;

    std::string GetName() const override { return "MiniMax"; }
    std::string GetId() const override { return "minimax"; }

    bool RenderConfigurationUI() override;

    void LoadConfig(const nlohmann::json& json) override;
    nlohmann::json SaveConfig() const override;

    void LoadRemoteVoices() override;

    bool RenderVoiceSelector(const char* label, std::string* selectedVoiceId) override;

    std::vector<unsigned char> GenerateAudio(const std::string& text,
                                             const std::string& voiceId = "",
                                             const std::string& languageCode = "",
                                             const std::string& format = "mp3") override;

    const std::vector<MiniMaxVoice>& GetAvailableVoices() const { return m_AvailableVoices; }
    const std::string& GetCurrentVoiceId() const override { return m_VoiceId; }
    void SetVoiceId(const std::string& voiceId) override { m_VoiceId = voiceId; }

    const std::string& GetModel() const { return m_Model; }
    void SetModel(const std::string& model) { m_Model = model; }

private:

    std::vector<unsigned char> ConvertMp3ToOgg(const std::vector<unsigned char>& mp3Data);

    std::string m_ApiKey;
    std::string m_VoiceId;
    std::string m_Model = "speech-2.8-hd";

    std::vector<MiniMaxVoice> m_AvailableVoices;
    bool m_IsLoadingVoices = false;
    std::string m_StatusMessage;
    std::atomic<bool> m_VoicesUpdated{false};
  };

} // namespace Image2Card::AI
