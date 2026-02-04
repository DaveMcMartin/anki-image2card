#include "ai/native/NativeAudioProviderInternal.h"

#if defined(__linux__) && !defined(__ANDROID__)
#include <iostream>
#include <libspeechd.h>
#include <vector>

namespace Image2Card::AI
{
  class LinuxNativeAudioProviderImpl : public NativeAudioProviderImpl
  {
public:

    LinuxNativeAudioProviderImpl() = default;
    ~LinuxNativeAudioProviderImpl() override = default;

    const std::string& GetCurrentVoiceId() const override { return m_CurrentVoiceId; }
    void SetVoiceId(const std::string& voiceId) override { m_CurrentVoiceId = voiceId; }

    std::vector<unsigned char> GenerateAudio(const std::string& text,
                                             const std::string& voiceId,
                                             const std::string& languageCode,
                                             const std::string& format) override
    {
      SPDConnection* conn = spd_open("anki-image2card", "main", NULL, SPD_MODE_SINGLE);
      if (!conn) {
        return {};
      }

      spd_set_language(conn, "ja");
      spd_say(conn, SPD_MESSAGE, text.c_str());
      spd_close(conn);

      return {};
    }

private:

    std::string m_CurrentVoiceId;
  };

  std::unique_ptr<NativeAudioProviderImpl> CreateNativeAudioProviderImpl()
  {
    return std::make_unique<LinuxNativeAudioProviderImpl>();
  }
} // namespace Image2Card::AI
#endif
