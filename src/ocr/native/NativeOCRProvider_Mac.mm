#include "ocr/native/NativeOCRProviderInternal.h"

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>
#include <vector>
#include <string>

namespace Image2Card::OCR
{
  class MacNativeOCRProviderImpl : public NativeOCRProviderImpl
  {
public:

    MacNativeOCRProviderImpl() = default;
    ~MacNativeOCRProviderImpl() override = default;

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) override
    {
      @autoreleasepool {
        if (imageBuffer.empty()) {
          return "";
        }

        NSData* imageData = [NSData dataWithBytes:imageBuffer.data() length:imageBuffer.size()];
        if (!imageData) {
          return "";
        }

        NSError* error = nil;
        VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithData:imageData options:@{}];

        __block std::string recognizedText;

        VNRecognizeTextRequest* request = [[VNRecognizeTextRequest alloc] 
          initWithCompletionHandler:^(VNRequest* request, NSError* error) {
            if (error) {
              return;
            }

            for (VNRecognizedTextObservation* observation in request.results) {
              VNRecognizedText* topCandidate = [observation topCandidates:1].firstObject;
              if (topCandidate) {
                NSString* text = topCandidate.string;
                if (recognizedText.length() > 0) {
                  recognizedText += "\n";
                }
                recognizedText += [text UTF8String];
              }
            }
          }];

        request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
        request.usesLanguageCorrection = YES;

        NSArray<NSString*>* languages = @[@"ja-JP", @"en-US"];
        request.recognitionLanguages = languages;

        [handler performRequests:@[request] error:&error];

        if (error) {
          return "";
        }

        return recognizedText;
      }
    }

    bool IsInitialized() const override
    {
      if (@available(macOS 10.15, *)) {
        return true;
      }
      return false;
    }
  };

  std::unique_ptr<NativeOCRProviderImpl> CreateNativeOCRProviderImpl()
  {
    return std::make_unique<MacNativeOCRProviderImpl>();
  }
}
#endif
