#include "ai/native/NativeAudioProviderInternal.h"

#ifdef _WIN32
#include <atlbase.h>
#include <iostream>
#include <sapi.h>
#include <sphelper.h>
#include <string>
#include <vector>

#pragma comment(lib, "sapi.lib")
#pragma comment(lib, "ole32.lib")

namespace Image2Card::AI
{
  class WindowsNativeAudioProviderImpl : public NativeAudioProviderImpl
  {
public:

    WindowsNativeAudioProviderImpl() { CoInitialize(NULL); }

    ~WindowsNativeAudioProviderImpl() override { CoUninitialize(); }

    const std::string& GetCurrentVoiceId() const override { return m_CurrentVoiceId; }
    void SetVoiceId(const std::string& voiceId) override { m_CurrentVoiceId = voiceId; }

    std::vector<unsigned char> GenerateAudio(const std::string& text,
                                             const std::string& voiceId,
                                             const std::string& languageCode,
                                             const std::string& format) override
    {
      CComPtr<ISpVoice> cpVoice;
      HRESULT hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
      if (FAILED(hr))
        return {};

      CComPtr<IEnumSpObjectTokens> cpEnum;
      hr = SpEnumTokens(SPCAT_VOICES, L"Language=411", NULL, &cpEnum);

      CComPtr<ISpObjectToken> cpToken;
      if (SUCCEEDED(hr) && cpEnum) {
        cpEnum->Next(1, &cpToken, NULL);
      }

      if (cpToken) {
        cpVoice->SetVoice(cpToken);
      }

      CComPtr<ISpStream> cpStream;
      CSpStreamFormat originalFmt;
      originalFmt.Assign(SPSF_44kHz16BitMono);

      CComPtr<IStream> cpBaseStream;
      CreateStreamOnHGlobal(NULL, TRUE, &cpBaseStream);
      hr = cpStream.CoCreateInstance(CLSID_SpStream);
      if (FAILED(hr))
        return {};

      hr = cpStream->SetBaseStream(cpBaseStream, originalFmt.FormatId(), originalFmt.WaveFormatExPtr());
      if (FAILED(hr))
        return {};

      hr = cpVoice->SetOutput(cpStream, TRUE);
      if (FAILED(hr))
        return {};

      std::wstring wText;
      int len = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
      if (len > 0) {
        wText.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wText[0], len);
      }

      cpVoice->Speak(wText.c_str(), SPF_DEFAULT, NULL);

      LARGE_INTEGER liZero = {0};
      ULARGE_INTEGER uliSize;
      cpStream->Seek(liZero, STREAM_SEEK_END, &uliSize);

      std::vector<unsigned char> data;
      data.resize((size_t) uliSize.QuadPart);

      LARGE_INTEGER liBegin = {0};
      cpStream->Seek(liBegin, STREAM_SEEK_SET, NULL);

      ULONG bytesRead = 0;
      cpStream->Read(data.data(), (ULONG) data.size(), &bytesRead);

      return data;
    }

private:

    std::string m_CurrentVoiceId;
  };

  std::unique_ptr<NativeAudioProviderImpl> CreateNativeAudioProviderImpl()
  {
    return std::make_unique<WindowsNativeAudioProviderImpl>();
  }

} // namespace Image2Card::AI
#endif
