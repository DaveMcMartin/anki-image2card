#pragma once

#include <string>
#include <vector>

namespace Image2Card::OCR
{

  class IOCRProvider
  {
public:

    virtual ~IOCRProvider() = default;

    virtual std::string GetName() const = 0;

    virtual std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) = 0;

    virtual bool IsInitialized() const = 0;
  };

} // namespace Image2Card::OCR