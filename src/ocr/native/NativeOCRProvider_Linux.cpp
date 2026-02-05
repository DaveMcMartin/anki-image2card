#include "ocr/native/NativeOCRProviderInternal.h"

#if defined(__linux__) && !defined(__ANDROID__)
#include <string>
#include <vector>

namespace Image2Card::OCR
{
  class LinuxNativeOCRProviderImpl : public NativeOCRProviderImpl
  {
public:

    LinuxNativeOCRProviderImpl() = default;
    ~LinuxNativeOCRProviderImpl() override = default;

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) override { return ""; }

    bool IsInitialized() const override { return false; }
  };

  std::unique_ptr<NativeOCRProviderImpl> CreateNativeOCRProviderImpl()
  {
    return std::make_unique<LinuxNativeOCRProviderImpl>();
  }

} // namespace Image2Card::OCR
#endif
