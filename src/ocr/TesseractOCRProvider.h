#pragma once

#include <string>
#include <vector>

namespace tesseract
{
  class TessBaseAPI;
}

namespace Image2Card::OCR
{

  enum class TesseractOrientation
  {
    Horizontal,
    Vertical
  };

  class TesseractOCRProvider
  {
public:

    TesseractOCRProvider();
    ~TesseractOCRProvider();

    TesseractOCRProvider(const TesseractOCRProvider&) = delete;
    TesseractOCRProvider& operator=(const TesseractOCRProvider&) = delete;

    bool Initialize(const std::string& tessDataPath, const std::string& language);

    void SetOrientation(TesseractOrientation orientation);
    TesseractOrientation GetOrientation() const { return m_Orientation; }

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer);

    bool IsInitialized() const { return m_IsInitialized; }

private:

    tesseract::TessBaseAPI* m_TessAPI;
    bool m_IsInitialized;
    TesseractOrientation m_Orientation;
  };

} // namespace Image2Card::OCR
