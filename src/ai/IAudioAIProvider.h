#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Image2Card::AI
{

  class IAudioAIProvider
  {
public:

    virtual ~IAudioAIProvider() = default;

    virtual std::string GetName() const = 0;
    virtual std::string GetId() const = 0;

    virtual bool RenderConfigurationUI() = 0;

    virtual void LoadConfig(const nlohmann::json& json) = 0;
    virtual nlohmann::json SaveConfig() const = 0;

    virtual void LoadRemoteVoices() = 0;

    virtual bool RenderVoiceSelector(const char* label, std::string* selectedVoiceId) = 0;

    virtual const std::string& GetCurrentVoiceId() const = 0;
    virtual void SetVoiceId(const std::string& voiceId) = 0;

    virtual std::vector<unsigned char>
    GenerateAudio(const std::string& text, const std::string& voiceId = "", const std::string& languageCode = "") = 0;
  };

} // namespace Image2Card::AI
