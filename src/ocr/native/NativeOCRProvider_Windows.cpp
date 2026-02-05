#include "ocr/native/NativeOCRProviderInternal.h"

#ifdef _WIN32
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace winrt::Windows::Media::Ocr;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Globalization;

namespace Image2Card::OCR
{
  class WindowsNativeOCRProviderImpl : public NativeOCRProviderImpl
  {
private:

    OcrEngine m_OcrEngine{nullptr};
    bool m_IsInitialized{false};

    OcrEngine CreateOcrEngine()
    {
      try {
        Language language{L"ja-JP"};

        if (OcrEngine::IsLanguageSupported(language)) {
          return OcrEngine::TryCreateFromLanguage(language);
        }

        Language englishLanguage{L"en-US"};
        if (OcrEngine::IsLanguageSupported(englishLanguage)) {
          return OcrEngine::TryCreateFromLanguage(englishLanguage);
        }

        return OcrEngine::TryCreateFromUserProfileLanguages();
      } catch (...) {
        return nullptr;
      }
    }

    std::vector<Language> GetAvailableLanguages()
    {
      std::vector<Language> languages;

      try {
        Language japaneseLanguage{L"ja-JP"};
        Language englishLanguage{L"en-US"};

        if (OcrEngine::IsLanguageSupported(japaneseLanguage)) {
          languages.push_back(japaneseLanguage);
        }

        if (OcrEngine::IsLanguageSupported(englishLanguage)) {
          languages.push_back(englishLanguage);
        }
      } catch (...) {}

      return languages;
    }

    SoftwareBitmap DecodeImage(const std::vector<unsigned char>& imageBuffer)
    {
      try {
        if (imageBuffer.empty()) {
          return nullptr;
        }

        InMemoryRandomAccessStream memoryStream;
        memoryStream.WriteAsync({imageBuffer.data(), imageBuffer.size()}).get();
        memoryStream.Seek(0);

        SoftwareBitmap softwareBitmap = BitmapDecoder::CreateAsync(memoryStream).get().GetSoftwareBitmapAsync().get();

        if (!softwareBitmap) {
          return nullptr;
        }

        BitmapPixelFormat requiredFormat{BitmapPixelFormat::Bgra8};
        if (softwareBitmap.BitmapPixelFormat() != requiredFormat) {
          softwareBitmap = SoftwareBitmap::Convert(softwareBitmap, requiredFormat);
        }

        return softwareBitmap;
      } catch (...) {
        return nullptr;
      }
    }

    std::string ExtractTextFromBitmap(const SoftwareBitmap& bitmap)
    {
      try {
        if (!bitmap || !m_OcrEngine) {
          return "";
        }

        OcrResult result = m_OcrEngine.RecognizeAsync(bitmap).get();

        if (!result) {
          return "";
        }

        std::ostringstream textStream;
        bool firstLine = true;

        for (const auto& line : result.Lines()) {
          if (!firstLine) {
            textStream << "\n";
          }

          bool firstWord = true;
          for (const auto& word : line.Words()) {
            if (!firstWord) {
              textStream << " ";
            }
            std::string wordText = winrt::to_string(word.Text());
            textStream << wordText;
            firstWord = false;
          }

          firstLine = false;
        }

        return textStream.str();
      } catch (...) {
        return "";
      }
    }

public:

    WindowsNativeOCRProviderImpl()
    {
      try {
        m_OcrEngine = CreateOcrEngine();
        m_IsInitialized = (m_OcrEngine != nullptr);
      } catch (...) {
        m_IsInitialized = false;
      }
    }

    ~WindowsNativeOCRProviderImpl() override = default;

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) override
    {
      try {
        if (!m_IsInitialized || !m_OcrEngine) {
          return "";
        }

        SoftwareBitmap bitmap = DecodeImage(imageBuffer);
        if (!bitmap) {
          return "";
        }

        return ExtractTextFromBitmap(bitmap);
      } catch (...) {
        return "";
      }
    }

    bool IsInitialized() const override { return m_IsInitialized; }
  };

  std::unique_ptr<NativeOCRProviderImpl> CreateNativeOCRProviderImpl()
  {
    return std::make_unique<WindowsNativeOCRProviderImpl>();
  }

} // namespace Image2Card::OCR
#endif
