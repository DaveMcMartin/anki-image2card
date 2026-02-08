#include "ocr/native/NativeOCRProviderInternal.h"

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>
#import <ImageIO/ImageIO.h>
#import <CoreServices/CoreServices.h>
#include <vector>
#include <string>
#include <cmath>
#include <optional>
#include <memory>
#include <algorithm>
#include "core/Logger.h"

namespace Image2Card::OCR
{
  namespace {
    constexpr float MINIMUM_CONFIDENCE_THRESHOLD = 0.1f;
    constexpr float VERTICAL_ASPECT_RATIO_THRESHOLD = 1.1f;
    constexpr size_t MAX_TOP_CANDIDATES = 10;
  }

  template<typename T>
  class CoreFoundationPtr
  {
  public:
    using Deleter = void(*)(T);
    
    CoreFoundationPtr() = default;
    
    explicit CoreFoundationPtr(T ptr, Deleter deleter = nullptr) 
      : m_Ptr(ptr), m_Deleter(deleter) {}
    
    ~CoreFoundationPtr() {
      Reset();
    }
    
    CoreFoundationPtr(const CoreFoundationPtr&) = delete;
    CoreFoundationPtr& operator=(const CoreFoundationPtr&) = delete;
    
    CoreFoundationPtr(CoreFoundationPtr&& other) noexcept 
      : m_Ptr(other.m_Ptr), m_Deleter(other.m_Deleter) {
      other.m_Ptr = nullptr;
    }
    
    CoreFoundationPtr& operator=(CoreFoundationPtr&& other) noexcept {
      if (this != &other) {
        Reset();
        m_Ptr = other.m_Ptr;
        m_Deleter = other.m_Deleter;
        other.m_Ptr = nullptr;
      }
      return *this;
    }
    
    void Reset(T ptr = nullptr) {
      if (m_Ptr && m_Deleter) {
        m_Deleter(m_Ptr);
      }
      m_Ptr = ptr;
    }
    
    T Get() const { return m_Ptr; }
    T Release() {
      T tmp = m_Ptr;
      m_Ptr = nullptr;
      return tmp;
    }
    
    explicit operator bool() const { return m_Ptr != nullptr; }
    
  private:
    T m_Ptr = nullptr;
    Deleter m_Deleter = nullptr;
  };

  struct CGImageDeleter {
    void operator()(CGImageRef ptr) const { if (ptr) CGImageRelease(ptr); }
  };
  
  struct CFTypeDeleter {
    void operator()(CFTypeRef ptr) const { if (ptr) CFRelease(ptr); }
  };
  
  struct CGContextDeleter {
    void operator()(CGContextRef ptr) const { if (ptr) CGContextRelease(ptr); }
  };
  
  template<typename T, typename Deleter>
  class ScopedCFPtr
  {
  public:
    ScopedCFPtr() = default;
    
    explicit ScopedCFPtr(T ptr) : m_Ptr(ptr) {}
    
    ~ScopedCFPtr() {
      Reset();
    }
    
    ScopedCFPtr(const ScopedCFPtr&) = delete;
    ScopedCFPtr& operator=(const ScopedCFPtr&) = delete;
    
    ScopedCFPtr(ScopedCFPtr&& other) noexcept : m_Ptr(other.m_Ptr) {
      other.m_Ptr = nullptr;
    }
    
    ScopedCFPtr& operator=(ScopedCFPtr&& other) noexcept {
      if (this != &other) {
        Reset();
        m_Ptr = other.m_Ptr;
        other.m_Ptr = nullptr;
      }
      return *this;
    }
    
    void Reset(T ptr = nullptr) {
      if (m_Ptr) {
        Deleter{}(m_Ptr);
      }
      m_Ptr = ptr;
    }
    
    T Get() const { return m_Ptr; }
    T Release() {
      T tmp = m_Ptr;
      m_Ptr = nullptr;
      return tmp;
    }
    
    explicit operator bool() const { return m_Ptr != nullptr; }
    
  private:
    T m_Ptr = nullptr;
  };

  using CGImagePtr = ScopedCFPtr<CGImageRef, CGImageDeleter>;
  using CGImageSourcePtr = ScopedCFPtr<CGImageSourceRef, CFTypeDeleter>;
  using CGContextPtr = ScopedCFPtr<CGContextRef, CGContextDeleter>;

  struct ImageMetadata
  {
    size_t Width = 0;
    size_t Height = 0;
    size_t BitsPerComponent = 0;
    size_t BitsPerPixel = 0;
    bool IsVertical = false;
  };

  struct RecognitionResult
  {
    std::string Text;
    float AverageConfidence = 0.0f;
    size_t ObservationCount = 0;
  };

  class VisionRequestConfigurator
  {
  public:
    static void ConfigureForJapaneseOCR(VNRecognizeTextRequest* request)
    {
      if (!request) {
        AF_ERROR("[VisionRequestConfigurator] Null request");
        return;
      }

      request.recognitionLevel = VNRequestTextRecognitionLevelAccurate;
      AF_INFO("[VisionRequestConfigurator] Recognition level: Accurate");

      SetAPIRevision(request);
      ConfigureLanguages(request);
      ConfigureTextDetection(request);
    }

  private:
    static void SetAPIRevision(VNRecognizeTextRequest* request)
    {
      if (@available(macOS 13.0, *)) {
        request.revision = VNRecognizeTextRequestRevision3;
        AF_INFO("[VisionRequestConfigurator] API revision: 3 (macOS 13.0+)");
      } else if (@available(macOS 11.0, *)) {
        request.revision = VNRecognizeTextRequestRevision2;
        AF_INFO("[VisionRequestConfigurator] API revision: 2 (macOS 11.0+)");
      } else {
        request.revision = VNRecognizeTextRequestRevision1;
        AF_INFO("[VisionRequestConfigurator] API revision: 1 (macOS 10.15+)");
      }
    }

    static void ConfigureLanguages(VNRecognizeTextRequest* request)
    {
      NSError* error = nil;
      NSArray<NSString*>* supportedLanguages = nil;
      
      if (@available(macOS 11.0, *)) {
        supportedLanguages = [request supportedRecognitionLanguagesAndReturnError:&error];
        
        if (error) {
          AF_ERROR("[VisionRequestConfigurator] Failed to query supported languages: {}", 
                  std::string([error.localizedDescription UTF8String]));
          return;
        }

        LogSupportedLanguages(supportedLanguages);
      }

      request.recognitionLanguages = @[@"ja-JP"];
      request.automaticallyDetectsLanguage = NO;
      
      AF_INFO("[VisionRequestConfigurator] Recognition languages: ja-JP (forced)");
      AF_INFO("[VisionRequestConfigurator] Automatic language detection: OFF");
    }

    static void ConfigureTextDetection(VNRecognizeTextRequest* request)
    {
      request.usesLanguageCorrection = YES;
      request.minimumTextHeight = 0.0;
      
      AF_INFO("[VisionRequestConfigurator] Language correction: ON");
      AF_INFO("[VisionRequestConfigurator] Minimum text height: 0.0");
    }

    static void LogSupportedLanguages(NSArray<NSString*>* supportedLanguages)
    {
      if (!supportedLanguages) {
        return;
      }

      bool hasJapanese = false;
      for (NSString* lang in supportedLanguages) {
        if ([lang hasPrefix:@"ja"]) {
          hasJapanese = true;
          break;
        }
      }
      
      if (!hasJapanese) {
        AF_ERROR("[VisionRequestConfigurator] Japanese not in supported languages!");
      }
      
      AF_DEBUG("[VisionRequestConfigurator] Supported languages count: {}", supportedLanguages.count);
    }
  };

  class TextRecognitionResultProcessor
  {
  public:
    static RecognitionResult ProcessResults(NSArray* results)
    {
      RecognitionResult result;
      
      if (!results || results.count == 0) {
        AF_WARN("[TextRecognitionResultProcessor] No observations from Vision API");
        return result;
      }

      result.ObservationCount = results.count;
      AF_INFO("[TextRecognitionResultProcessor] Processing {} observations", results.count);

      std::vector<std::string> textLines;
      std::vector<float> confidences;

      for (VNRecognizedTextObservation* observation in results) {
        ProcessObservation(observation, textLines, confidences);
      }

      if (!textLines.empty()) {
        result.Text = JoinTextLines(textLines);
        result.AverageConfidence = CalculateAverageConfidence(confidences);
        
        AF_INFO("[TextRecognitionResultProcessor] Extracted {} lines, {} chars, confidence: {:.3f}",
                textLines.size(), result.Text.length(), result.AverageConfidence);
      } else {
        AF_WARN("[TextRecognitionResultProcessor] No text met threshold {:.2f}", 
                MINIMUM_CONFIDENCE_THRESHOLD);
      }

      return result;
    }

  private:
    static void ProcessObservation(VNRecognizedTextObservation* observation,
                                   std::vector<std::string>& textLines,
                                   std::vector<float>& confidences)
    {
      NSArray<VNRecognizedText*>* candidates = [observation topCandidates:MAX_TOP_CANDIDATES];
      
      if (!candidates || candidates.count == 0) {
        return;
      }

      LogCandidates(candidates);

      VNRecognizedText* topCandidate = candidates[0];
      NSString* text = topCandidate.string;
      float confidence = topCandidate.confidence;

      if (confidence >= MINIMUM_CONFIDENCE_THRESHOLD) {
        std::string textStr = [text UTF8String];
        textLines.push_back(textStr);
        confidences.push_back(confidence);
        
        AF_INFO("[TextRecognitionResultProcessor] Accepted: '{}' ({:.3f})", 
                 textStr, confidence);
      } else {
        AF_DEBUG("[TextRecognitionResultProcessor] Rejected: '{}' ({:.3f})",
                 std::string([text UTF8String]), confidence);
      }
    }

    static void LogCandidates(NSArray<VNRecognizedText*>* candidates)
    {
      for (size_t i = 0; i < candidates.count && i < 3; ++i) {
        VNRecognizedText* candidate = candidates[i];
        AF_DEBUG("[TextRecognitionResultProcessor] Candidate {}: '{}' ({:.3f})",
                 i + 1, std::string([candidate.string UTF8String]), candidate.confidence);
      }
    }

    static std::string JoinTextLines(const std::vector<std::string>& lines)
    {
      std::string result;
      for (const auto& line : lines) {
        if (!result.empty()) {
          result += "\n";
        }
        result += line;
      }
      return result;
    }

    static float CalculateAverageConfidence(const std::vector<float>& confidences)
    {
      if (confidences.empty()) {
        return 0.0f;
      }
      
      float sum = 0.0f;
      for (float conf : confidences) {
        sum += conf;
      }
      return sum / static_cast<float>(confidences.size());
    }
  };

  class ImageProcessor
  {
  public:
    static std::optional<ImageMetadata> ExtractMetadata(CGImageRef image)
    {
      if (!image) {
        AF_ERROR("[ImageProcessor] Null image");
        return std::nullopt;
      }

      ImageMetadata metadata;
      metadata.Width = CGImageGetWidth(image);
      metadata.Height = CGImageGetHeight(image);
      metadata.BitsPerComponent = CGImageGetBitsPerComponent(image);
      metadata.BitsPerPixel = CGImageGetBitsPerPixel(image);
      
      float aspectRatio = static_cast<float>(metadata.Height) / static_cast<float>(metadata.Width);
      metadata.IsVertical = aspectRatio > VERTICAL_ASPECT_RATIO_THRESHOLD;

      AF_INFO("[ImageProcessor] Dimensions: {}x{}", metadata.Width, metadata.Height);
      AF_INFO("[ImageProcessor] Bits per component: {}", metadata.BitsPerComponent);
      AF_INFO("[ImageProcessor] Bits per pixel: {}", metadata.BitsPerPixel);
      AF_INFO("[ImageProcessor] Aspect ratio: {:.2f}", aspectRatio);
      AF_INFO("[ImageProcessor] Orientation: {}", metadata.IsVertical ? "VERTICAL" : "HORIZONTAL");

      return metadata;
    }

    static CGImagePropertyOrientation DetermineOrientation(const ImageMetadata& metadata)
    {
      if (metadata.IsVertical) {
        AF_INFO("[ImageProcessor] Using orientation: Right (vertical text)");
        return kCGImagePropertyOrientationRight;
      } else {
        AF_INFO("[ImageProcessor] Using orientation: Up (horizontal text)");
        return kCGImagePropertyOrientationUp;
      }
    }

    static CGImagePtr LoadImageFromBuffer(const std::vector<unsigned char>& imageBuffer)
    {
      if (imageBuffer.empty()) {
        AF_ERROR("[ImageProcessor] Empty buffer");
        return CGImagePtr();
      }

      AF_INFO("[ImageProcessor] Loading image ({} bytes)", imageBuffer.size());

      NSData* imageData = [NSData dataWithBytes:imageBuffer.data() length:imageBuffer.size()];
      if (!imageData) {
        AF_ERROR("[ImageProcessor] NSData creation failed");
        return CGImagePtr();
      }

      CGImageSourcePtr imageSource(CGImageSourceCreateWithData((__bridge CFDataRef)imageData, nullptr));
      if (!imageSource) {
        AF_ERROR("[ImageProcessor] CGImageSource creation failed");
        return CGImagePtr();
      }

      size_t imageCount = CGImageSourceGetCount(imageSource.Get());
      if (imageCount == 0) {
        AF_ERROR("[ImageProcessor] No images in source");
        return CGImagePtr();
      }

      CGImageRef cgImage = CGImageSourceCreateImageAtIndex(imageSource.Get(), 0, nullptr);
      if (!cgImage) {
        AF_ERROR("[ImageProcessor] CGImage decode failed");
        return CGImagePtr();
      }

      AF_INFO("[ImageProcessor] Image loaded successfully");
      return CGImagePtr(cgImage);
    }
  };

  class VisionTextRecognizer
  {
  public:
    RecognitionResult PerformRecognition(CGImageRef image, CGImagePropertyOrientation orientation)
    {
      RecognitionResult result;
      
      if (!image) {
        AF_ERROR("[VisionTextRecognizer] Null image");
        return result;
      }

      AF_INFO("[VisionTextRecognizer] Starting OCR with orientation: {}", static_cast<int>(orientation));

      NSDictionary* handlerOptions = @{};

      VNImageRequestHandler* handler = [[VNImageRequestHandler alloc]
                                        initWithCGImage:image
                                        orientation:orientation
                                        options:handlerOptions];

      if (!handler) {
        AF_ERROR("[VisionTextRecognizer] VNImageRequestHandler creation failed");
        return result;
      }

      __block RecognitionResult blockResult;
      __block NSError* completionError = nil;

      VNRecognizeTextRequest* request = [[VNRecognizeTextRequest alloc]
        initWithCompletionHandler:^(VNRequest* req, NSError* error) {
          if (error) {
            completionError = error;
            AF_ERROR("[VisionTextRecognizer] Completion error: {}",
                     std::string([error.localizedDescription UTF8String]));
            return;
          }

          blockResult = TextRecognitionResultProcessor::ProcessResults(req.results);
        }];

      VisionRequestConfigurator::ConfigureForJapaneseOCR(request);

      AF_INFO("[VisionTextRecognizer] Executing Vision request");
      NSError* requestError = nil;
      BOOL success = [handler performRequests:@[request] error:&requestError];

      if (requestError) {
        AF_ERROR("[VisionTextRecognizer] Request error: {}",
                 std::string([requestError.localizedDescription UTF8String]));
        AF_ERROR("[VisionTextRecognizer] Error domain: {}", 
                 std::string([requestError.domain UTF8String]));
        AF_ERROR("[VisionTextRecognizer] Error code: {}", requestError.code);
        return result;
      }

      if (!success) {
        AF_ERROR("[VisionTextRecognizer] Request failed");
        return result;
      }

      if (completionError) {
        return result;
      }

      AF_INFO("[VisionTextRecognizer] OCR completed: {} chars, confidence: {:.3f}",
              blockResult.Text.length(), blockResult.AverageConfidence);

      return blockResult;
    }
  };

  class MacNativeOCRProviderImpl : public NativeOCRProviderImpl
  {
  public:
    MacNativeOCRProviderImpl() = default;
    ~MacNativeOCRProviderImpl() override = default;

    std::string ExtractTextFromImage(const std::vector<unsigned char>& imageBuffer) override
    {
      @autoreleasepool {
        AF_INFO("[MacNativeOCR] Starting OCR");
        
        CGImagePtr image = ImageProcessor::LoadImageFromBuffer(imageBuffer);
        if (!image) {
          return "";
        }

        auto metadata = ImageProcessor::ExtractMetadata(image.Get());
        if (!metadata) {
          return "";
        }

        CGImagePropertyOrientation orientation = ImageProcessor::DetermineOrientation(*metadata);

        VisionTextRecognizer recognizer;
        RecognitionResult result = recognizer.PerformRecognition(image.Get(), orientation);

        if (!result.Text.empty()) {
          AF_INFO("[MacNativeOCR] SUCCESS: {} chars, confidence: {:.3f}", 
                  result.Text.length(), result.AverageConfidence);
          return result.Text;
        }

        AF_ERROR("[MacNativeOCR] FAILED: No text extracted");
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
