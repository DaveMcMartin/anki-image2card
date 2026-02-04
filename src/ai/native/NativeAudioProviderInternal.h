#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Image2Card::AI
{
  class NativeAudioProviderImpl
  {
public:

    virtual ~NativeAudioProviderImpl() = default;

    virtual void LoadConfig(const nlohmann::json& json) {}
    virtual nlohmann::json SaveConfig() const { return nlohmann::json::object(); }

    virtual void LoadRemoteVoices() {}
    virtual bool RenderConfigurationUI() { return false; }
    virtual bool RenderVoiceSelector(const char* label, std::string* selectedVoiceId) { return false; }

    virtual const std::string& GetCurrentVoiceId() const = 0;
    virtual void SetVoiceId(const std::string& voiceId) = 0;

    virtual std::vector<unsigned char> GenerateAudio(const std::string& text,
                                                     const std::string& voiceId,
                                                     const std::string& languageCode,
                                                     const std::string& format) = 0;
  };

  std::unique_ptr<NativeAudioProviderImpl> CreateNativeAudioProviderImpl();

} // namespace Image2Card::AI
