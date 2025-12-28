#include "ocr/TesseractOCRProvider.h"

#include <allheaders.h>
#include <cstring>
#include <tesseract/baseapi.h>

#include "core/Logger.h"

namespace Image2Card::OCR
{

  TesseractOCRProvider::TesseractOCRProvider()
      : m_TessAPI(std::make_unique<tesseract::TessBaseAPI>())
      , m_IsInitialized(false)
      , m_Orientation(TesseractOrientation::Horizontal)
  {}

  TesseractOCRProvider::~TesseractOCRProvider()
  {
    if (m_TessAPI) {
      m_TessAPI->End();
    }
  }

  bool TesseractOCRProvider::Initialize(const std::string& tessDataPath, const std::string& language)
  {
    if (!m_TessAPI) {
      AF_ERROR("Tesseract API is null");
      return false;
    }

    // Initialize tesseract-ocr with the specified language
    if (m_TessAPI->Init(tessDataPath.c_str(), language.c_str())) {
      AF_ERROR("Could not initialize Tesseract with language: {} at path: {}", language, tessDataPath);
      m_IsInitialized = false;
      return false;
    }

    m_IsInitialized = true;
    AF_INFO("Tesseract initialized successfully with language: {}", language);

    return true;
  }

  void TesseractOCRProvider::SetOrientation(TesseractOrientation orientation)
  {
    m_Orientation = orientation;
  }

  std::string TesseractOCRProvider::ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer)
  {
    if (!m_IsInitialized) {
      AF_ERROR("Tesseract is not initialized");
      return "";
    }

    if (imageBuffer.empty()) {
      AF_ERROR("Image buffer is empty");
      return "";
    }

    // Load image from memory using Leptonica
    Pix* image = pixReadMem(imageBuffer.data(), imageBuffer.size());
    if (!image) {
      AF_ERROR("Failed to load image from buffer");
      return "";
    }

    // Clear previous recognition state to ensure clean OCR
    m_TessAPI->Clear();

    // Set the image for OCR
    m_TessAPI->SetImage(image);

    // Set page segmentation mode based on orientation
    if (m_Orientation == TesseractOrientation::Vertical) {
      // PSM_SINGLE_BLOCK_VERT_TEXT (5) for vertical text
      m_TessAPI->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK_VERT_TEXT);
    } else {
      // PSM_AUTO (3) for automatic detection, good for horizontal text
      m_TessAPI->SetPageSegMode(tesseract::PSM_AUTO);
    }

    // Perform OCR
    char* outText = m_TessAPI->GetUTF8Text();

    std::string result;
    if (outText) {
      result = std::string(outText);
      delete[] outText;
    } else {
      AF_WARN("Tesseract OCR returned null text");
    }

    // Clean up
    if (image) {
      pixDestroy(&image);
    }

    AF_INFO("Tesseract OCR extracted {} characters", result.length());
    return result;
  }

} // namespace Image2Card::OCR
