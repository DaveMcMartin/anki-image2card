#include "ocr/native/NativeOCRProviderInternal.h"

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>
#import <ImageIO/ImageIO.h>
#import <CoreServices/CoreServices.h>
#include <vector>
#include <string>
#include <cmath>
#include "core/Logger.h"

namespace Image2Card::OCR
{
  class MacNativeOCRProviderImpl : public NativeOCRProviderImpl
  {
public:

    MacNativeOCRProviderImpl() = default;
    ~MacNativeOCRProviderImpl() override = default;

    std::string PerformOCRWithHandler(VNImageRequestHandler* handler, bool tryVertical, bool useFast = false) {
      NSError* error = nil;
      __block std::string recognizedText;
      __block NSError* completionError = nil;
      __block NSUInteger observationCount = 0;

      VNRecognizeTextRequest* request = [[VNRecognizeTextRequest alloc] 
        initWithCompletionHandler:^(VNRequest* request, NSError* error) {
          if (error) {
            completionError = error;
            AF_ERROR("[NativeOCR-Mac] Vision completion error: {}",
                     std::string([error.localizedDescription UTF8String]));
            return;
          }
          
          if (!request.results || request.results.count == 0) {
            return;
          }
          
          observationCount = request.results.count;

          for (VNRecognizedTextObservation* observation in request.results) {
            NSArray<VNRecognizedText*>* candidates = [observation topCandidates:5];
            for (VNRecognizedText* candidate in candidates) {
              NSString* text = candidate.string;
              float confidence = candidate.confidence;
              AF_DEBUG("[NativeOCR-Mac] Candidate: '{}' (confidence: {:.2f})", 
                       std::string([text UTF8String]), confidence);
              
              if (confidence > 0.1) {
                if (recognizedText.length() > 0) {
                  recognizedText += "\n";
                }
                recognizedText += [text UTF8String];
                break;
              }
            }
          }
        }];

      request.recognitionLevel = useFast ? VNRequestTextRecognitionLevelFast : VNRequestTextRecognitionLevelAccurate;
      request.usesLanguageCorrection = YES;
      request.automaticallyDetectsLanguage = YES;
      request.minimumTextHeight = 0.0;
      
      if (@available(macOS 13.0, *)) {
        request.revision = VNRecognizeTextRequestRevision3;
        AF_INFO("[NativeOCR-Mac] Using VNRecognizeTextRequestRevision3");
      } else if (@available(macOS 11.0, *)) {
        request.revision = VNRecognizeTextRequestRevision2;
        AF_INFO("[NativeOCR-Mac] Using VNRecognizeTextRequestRevision2");
      } else {
        AF_INFO("[NativeOCR-Mac] Using VNRecognizeTextRequestRevision1");
      }

      NSArray<NSString*>* supportedLanguages = nil;
      if (@available(macOS 11.0, *)) {
        supportedLanguages = [request supportedRecognitionLanguagesAndReturnError:&error];
      }
      
      if (supportedLanguages && !tryVertical) {
        bool japaneseSupported = false;
        for (NSString* lang in supportedLanguages) {
          if ([lang hasPrefix:@"ja"]) {
            japaneseSupported = true;
            break;
          }
        }
        
        if (japaneseSupported) {
          NSArray<NSString*>* languages = @[@"ja-JP", @"en-US"];
          request.recognitionLanguages = languages;
          AF_INFO("[NativeOCR-Mac] Set recognition languages: ja-JP, en-US");
        }
      }

      BOOL success = [handler performRequests:@[request] error:&error];

      if (error) {
        AF_ERROR("[NativeOCR-Mac] Vision request error: {}", 
                 std::string([error.localizedDescription UTF8String]));
        return "";
      }
      
      if (!success) {
        AF_ERROR("[NativeOCR-Mac] Vision request failed without error");
        return "";
      }
      
      if (completionError) {
        return "";
      }

      AF_INFO("[NativeOCR-Mac] {} orientation ({}): {} observations, {} characters", 
              tryVertical ? "Vertical" : "Horizontal",
              useFast ? "fast" : "accurate",
              observationCount, recognizedText.length());

      return recognizedText;
    }

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) override
    {
      @autoreleasepool {
        AF_INFO("[NativeOCR-Mac] Starting text extraction, image buffer size: {} bytes", imageBuffer.size());
        
        if (imageBuffer.empty()) {
          AF_ERROR("[NativeOCR-Mac] Image buffer is empty");
          return "";
        }

        NSData* imageData = [NSData dataWithBytes:imageBuffer.data() length:imageBuffer.size()];
        if (!imageData) {
          AF_ERROR("[NativeOCR-Mac] Failed to create NSData from image buffer");
          return "";
        }
        
        AF_INFO("[NativeOCR-Mac] NSData created successfully, size: {} bytes", [imageData length]);
        
        CGImageSourceRef imageSource = CGImageSourceCreateWithData((__bridge CFDataRef)imageData, nil);
        if (imageSource) {
          size_t imageCount = CGImageSourceGetCount(imageSource);
          AF_INFO("[NativeOCR-Mac] Image source created, image count: {}", imageCount);
          
          if (imageCount > 0) {
            CGImageRef cgImage = CGImageSourceCreateImageAtIndex(imageSource, 0, nil);
            if (cgImage) {
              size_t width = CGImageGetWidth(cgImage);
              size_t height = CGImageGetHeight(cgImage);
              size_t bitsPerComponent = CGImageGetBitsPerComponent(cgImage);
              size_t bitsPerPixel = CGImageGetBitsPerPixel(cgImage);
              AF_INFO("[NativeOCR-Mac] Image decoded: {}x{}, {} bits/component, {} bits/pixel", 
                      width, height, bitsPerComponent, bitsPerPixel);
              
              bool isVertical = height > width * 1.5;
              AF_INFO("[NativeOCR-Mac] Image aspect ratio suggests {} text orientation", 
                      isVertical ? "vertical" : "horizontal");
              
              NSError* error = nil;
              VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] 
                                                initWithCGImage:cgImage 
                                                orientation:kCGImagePropertyOrientationUp
                                                options:@{}];
              
              if (!handler) {
                AF_ERROR("[NativeOCR-Mac] Failed to create VNImageRequestHandler");
                CGImageRelease(cgImage);
                CFRelease(imageSource);
                return "";
              }
              
              std::string horizontalText = PerformOCRWithHandler(handler, false, false);
              
              if (horizontalText.empty()) {
                AF_INFO("[NativeOCR-Mac] Trying fast recognition mode");
                horizontalText = PerformOCRWithHandler(handler, false, true);
              }
              
              if (horizontalText.empty() && isVertical) {
                AF_INFO("[NativeOCR-Mac] No horizontal text found, trying rotated image for vertical text");
                
                CGContextRef context = CGBitmapContextCreate(nil, height, width, 
                                                            bitsPerComponent, 0,
                                                            CGImageGetColorSpace(cgImage),
                                                            kCGImageAlphaPremultipliedLast);
                if (context) {
                  CGContextTranslateCTM(context, height, 0);
                  CGContextRotateCTM(context, M_PI_2);
                  CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
                  
                  CGImageRef rotatedImage = CGBitmapContextCreateImage(context);
                  if (rotatedImage) {
                    VNImageRequestHandler* rotatedHandler = [[VNImageRequestHandler alloc] 
                                                            initWithCGImage:rotatedImage
                                                            orientation:kCGImagePropertyOrientationUp
                                                            options:@{}];
                    if (rotatedHandler) {
                        std::string verticalText = PerformOCRWithHandler(rotatedHandler, true, false);
                        if (verticalText.empty()) {
                          AF_INFO("[NativeOCR-Mac] Trying fast recognition mode for vertical");
                          verticalText = PerformOCRWithHandler(rotatedHandler, true, true);
                        }
                        
                        if (!verticalText.empty()) {
                          CGContextRelease(context);
                          CGImageRelease(rotatedImage);
                          CGImageRelease(cgImage);
                          CFRelease(imageSource);
                          return verticalText;
                        }
                    }
                    CGImageRelease(rotatedImage);
                  }
                  CGContextRelease(context);
                }
              }
              
              CGImageRelease(cgImage);
              CFRelease(imageSource);
              return horizontalText;
            } else {
              AF_ERROR("[NativeOCR-Mac] Failed to create CGImage from image source");
            }
          }
          CFRelease(imageSource);
        } else {
          AF_ERROR("[NativeOCR-Mac] Failed to create CGImageSource - image data may be invalid");
        }

        return "";
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
