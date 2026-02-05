#include "ocr/NativeOCRProvider.h"

#include "ocr/native/NativeOCRProviderInternal.h"

namespace Image2Card::OCR
{
  struct NativeOCRProvider::Impl
  {
    std::unique_ptr<NativeOCRProviderImpl> m_PlatformImpl;

    Impl()
        : m_PlatformImpl(CreateNativeOCRProviderImpl())
    {}
  };

  NativeOCRProvider::NativeOCRProvider()
      : m_Impl(std::make_unique<Impl>())
  {}

  NativeOCRProvider::~NativeOCRProvider() = default;

  std::string NativeOCRProvider::ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer)
  {
    return m_Impl->m_PlatformImpl->ExtractTextFromImage(imageBuffer);
  }

  bool NativeOCRProvider::IsInitialized() const
  {
    return m_Impl->m_PlatformImpl->IsInitialized();
  }

} // namespace Image2Card::OCR
