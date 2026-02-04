#include "ai/NativeAudioProvider.h"

#include "ai/native/NativeAudioProviderInternal.h"

namespace Image2Card::AI
{
  struct NativeAudioProvider::Impl
  {
    std::unique_ptr<NativeAudioProviderImpl> m_PlatformImpl;

    Impl()
        : m_PlatformImpl(CreateNativeAudioProviderImpl())
    {}
  };

  NativeAudioProvider::NativeAudioProvider()
      : m_Impl(std::make_unique<Impl>())
  {}

  NativeAudioProvider::~NativeAudioProvider() = default;

  bool NativeAudioProvider::RenderConfigurationUI()
  {
    return m_Impl->m_PlatformImpl->RenderConfigurationUI();
  }

  void NativeAudioProvider::LoadConfig(const nlohmann::json& json)
  {
    m_Impl->m_PlatformImpl->LoadConfig(json);
  }

  nlohmann::json NativeAudioProvider::SaveConfig() const
  {
    return m_Impl->m_PlatformImpl->SaveConfig();
  }

  void NativeAudioProvider::LoadRemoteVoices()
  {
    m_Impl->m_PlatformImpl->LoadRemoteVoices();
  }

  bool NativeAudioProvider::RenderVoiceSelector(const char* label, std::string* selectedVoiceId)
  {
    return m_Impl->m_PlatformImpl->RenderVoiceSelector(label, selectedVoiceId);
  }

  const std::string& NativeAudioProvider::GetCurrentVoiceId() const
  {
    return m_Impl->m_PlatformImpl->GetCurrentVoiceId();
  }

  void NativeAudioProvider::SetVoiceId(const std::string& voiceId)
  {
    m_Impl->m_PlatformImpl->SetVoiceId(voiceId);
  }

  std::vector<unsigned char> NativeAudioProvider::GenerateAudio(const std::string& text,
                                                                const std::string& voiceId,
                                                                const std::string& languageCode,
                                                                const std::string& format)
  {
    return m_Impl->m_PlatformImpl->GenerateAudio(text, voiceId, languageCode, format);
  }

} // namespace Image2Card::AI
