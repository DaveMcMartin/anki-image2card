#pragma once

#include "ai/IAudioAIProvider.h"
#include <string>
#include <vector>

namespace Image2Card::AI
{

struct ElevenLabsVoice
{
    std::string Id;
    std::string Name;
};

class ElevenLabsAudioProvider : public IAudioAIProvider
{
public:
    ElevenLabsAudioProvider();
    ~ElevenLabsAudioProvider() override;

    std::string GetName() const override { return "ElevenLabs"; }
    std::string GetId() const override { return "elevenlabs"; }

    bool RenderConfigurationUI() override;

    void LoadConfig(const nlohmann::json& json) override;
    nlohmann::json SaveConfig() const override;

    void LoadRemoteVoices() override;

    bool RenderVoiceSelector(const char* label, std::string* selectedVoiceId) override;

    std::vector<unsigned char> GenerateAudio(const std::string& text, const std::string& voiceId = "", const std::string& languageCode = "") override;

    const std::vector<ElevenLabsVoice>& GetAvailableVoices() const { return m_AvailableVoices; }
    const std::string& GetCurrentVoiceId() const override { return m_VoiceId; }
    void SetVoiceId(const std::string& voiceId) override { m_VoiceId = voiceId; }

private:
    std::string m_ApiKey;
    std::string m_VoiceId;

    std::vector<ElevenLabsVoice> m_AvailableVoices;
    bool m_IsLoadingVoices = false;
    std::string m_StatusMessage;
};

} // namespace Image2Card::AI
