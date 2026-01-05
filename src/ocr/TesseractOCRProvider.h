#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ocr/IOCRProvider.h"

// Forward declare Pix struct from Leptonica
struct Pix;

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

  class TesseractOCRProvider : public IOCRProvider
  {
public:

    TesseractOCRProvider();
    ~TesseractOCRProvider();

    TesseractOCRProvider(const TesseractOCRProvider&) = delete;
    TesseractOCRProvider& operator=(const TesseractOCRProvider&) = delete;

    bool Initialize(const std::string& tessDataPath, const std::string& language);

    void SetOrientation(TesseractOrientation orientation);
    TesseractOrientation GetOrientation() const { return m_Orientation; }

    std::string GetName() const override { return "Tesseract (Local)"; }

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) override;

    bool IsInitialized() const override { return m_IsInitialized; }

private:

    std::unique_ptr<tesseract::TessBaseAPI> m_TessAPI;
    bool m_IsInitialized;
    TesseractOrientation m_Orientation;
  };

} // namespace Image2Card::OCR