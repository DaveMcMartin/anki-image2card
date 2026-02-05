#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ocr/IOCRProvider.h"

namespace Image2Card::OCR
{

  class NativeOCRProvider : public IOCRProvider
  {
public:

    NativeOCRProvider();
    ~NativeOCRProvider() override;

    NativeOCRProvider(const NativeOCRProvider&) = delete;
    NativeOCRProvider& operator=(const NativeOCRProvider&) = delete;

    std::string GetName() const override { return "Native OS"; }

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) override;

    bool IsInitialized() const override;

private:

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
  };

} // namespace Image2Card::OCR
