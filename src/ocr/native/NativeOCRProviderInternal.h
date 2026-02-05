#pragma once

#include <memory>
#include <string>
#include <vector>

namespace Image2Card::OCR
{
  class NativeOCRProviderImpl
  {
public:

    virtual ~NativeOCRProviderImpl() = default;

    virtual std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) = 0;

    virtual bool IsInitialized() const = 0;
  };

  std::unique_ptr<NativeOCRProviderImpl> CreateNativeOCRProviderImpl();

} // namespace Image2Card::OCR
