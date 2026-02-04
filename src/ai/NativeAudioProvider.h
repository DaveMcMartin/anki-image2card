#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ai/IAudioAIProvider.h"

namespace Image2Card::AI
{

  class NativeAudioProvider : public IAudioAIProvider
  {
public:

    NativeAudioProvider();
    ~NativeAudioProvider() override;

    std::string GetName() const override { return "Native TTS"; }
    std::string GetId() const override { return "native"; }

    bool RenderConfigurationUI() override;

    void LoadConfig(const nlohmann::json& json) override;
    nlohmann::json SaveConfig() const override;

    void LoadRemoteVoices() override;

    bool RenderVoiceSelector(const char* label, std::string* selectedVoiceId) override;

    const std::string& GetCurrentVoiceId() const override;
    void SetVoiceId(const std::string& voiceId) override;

    std::vector<unsigned char> GenerateAudio(const std::string& text,
                                             const std::string& voiceId = "",
                                             const std::string& languageCode = "",
                                             const std::string& format = "mp3") override;

private:

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
  };

} // namespace Image2Card::AI
