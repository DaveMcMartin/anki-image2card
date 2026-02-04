#include "ai/native/NativeAudioProviderInternal.h"

#ifdef __APPLE__
#import <AVFoundation/AVFoundation.h>
#include <vector>
#include <string>

@interface SpeechDelegate : NSObject <AVSpeechSynthesizerDelegate>
@property (nonatomic, assign) dispatch_semaphore_t semaphore;
@end

@implementation SpeechDelegate
- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer didFinishSpeechUtterance:(AVSpeechUtterance *)utterance {
    if (self.semaphore) {
        dispatch_semaphore_signal(self.semaphore);
    }
}
@end

namespace Image2Card::AI
{
  class MacNativeAudioProviderImpl : public NativeAudioProviderImpl
  {
  public:
    MacNativeAudioProviderImpl() {}
    ~MacNativeAudioProviderImpl() override {}

    const std::string& GetCurrentVoiceId() const override { return m_CurrentVoiceId; }
    void SetVoiceId(const std::string& voiceId) override { m_CurrentVoiceId = voiceId; }

    std::vector<unsigned char> GenerateAudio(const std::string& text,
                                             const std::string& voiceId,
                                             const std::string& languageCode,
                                             const std::string& format) override
    {
      @autoreleasepool {
          AVSpeechSynthesizer *synthesizer = [[AVSpeechSynthesizer alloc] init];
          SpeechDelegate *delegate = [[SpeechDelegate alloc] init];
          dispatch_semaphore_t sema = dispatch_semaphore_create(0);
          delegate.semaphore = sema;
          synthesizer.delegate = delegate;

          AVSpeechUtterance *utterance = [AVSpeechUtterance speechUtteranceWithString:[NSString stringWithUTF8String:text.c_str()]];
          
          NSString *lang = @"ja-JP";
          AVSpeechSynthesisVoice *voice = [AVSpeechSynthesisVoice voiceWithLanguage:lang];
          if (voice) {
              utterance.voice = voice;
          }
          
          std::vector<unsigned char> pcmData;
          auto *pcmDataPtr = &pcmData;
          __block AudioStreamBasicDescription asbd = {0};
          __block bool formatCaptured = false;

          [synthesizer writeUtterance:utterance toBufferCallback:^(AVAudioBuffer * _Nonnull buffer) {
              if (![buffer isKindOfClass:[AVAudioPCMBuffer class]]) return;
              AVAudioPCMBuffer *pcmBuffer = (AVAudioPCMBuffer *)buffer;

              if (!formatCaptured) {
                  asbd = *pcmBuffer.format.streamDescription;
                  formatCaptured = true;
              }
              
              if (pcmBuffer.frameLength == 0) return;
              
              const int numChannels = pcmBuffer.format.channelCount;
              const int numFrames = pcmBuffer.frameLength;
              
              if (pcmBuffer.floatChannelData) {
                  for (int i = 0; i < numFrames; i++) {
                       for (int c = 0; c < numChannels; c++) {
                           float sample = pcmBuffer.floatChannelData[c][i];
                           if (sample > 1.0f) sample = 1.0f;
                           if (sample < -1.0f) sample = -1.0f;
                           int16_t val = (int16_t)(sample * 32767.0f);
                           
                           pcmDataPtr->push_back((unsigned char)(val & 0xFF));
                           pcmDataPtr->push_back((unsigned char)((val >> 8) & 0xFF));
                       }
                  }
              }
          }];
          
          dispatch_semaphore_wait(sema, DISPATCH_TIME_FOREVER);
          
          if (pcmData.empty()) return {};
          
          uint32_t sampleRate = (uint32_t)asbd.mSampleRate;
          uint16_t numChannels = (uint16_t)asbd.mChannelsPerFrame;
          uint16_t bitsPerSample = 16;
          
          std::vector<unsigned char> wavFile;
          uint32_t dataSize = (uint32_t)pcmData.size();
          uint32_t fileSize = 36 + dataSize;
          
          wavFile.push_back('R'); wavFile.push_back('I'); wavFile.push_back('F'); wavFile.push_back('F');
          wavFile.push_back(fileSize & 0xFF); wavFile.push_back((fileSize >> 8) & 0xFF);
          wavFile.push_back((fileSize >> 16) & 0xFF); wavFile.push_back((fileSize >> 24) & 0xFF);
          
          wavFile.push_back('W'); wavFile.push_back('A'); wavFile.push_back('V'); wavFile.push_back('E');
          
          wavFile.push_back('f'); wavFile.push_back('m'); wavFile.push_back('t'); wavFile.push_back(' ');
          uint32_t fmtSize = 16;
          wavFile.push_back(fmtSize & 0xFF); wavFile.push_back((fmtSize >> 8) & 0xFF);
          wavFile.push_back((fmtSize >> 16) & 0xFF); wavFile.push_back((fmtSize >> 24) & 0xFF);
          
          uint16_t audioFormat = 1; 
          wavFile.push_back(audioFormat & 0xFF); wavFile.push_back((audioFormat >> 8) & 0xFF);
          
          wavFile.push_back(numChannels & 0xFF); wavFile.push_back((numChannels >> 8) & 0xFF);
          
          wavFile.push_back(sampleRate & 0xFF); wavFile.push_back((sampleRate >> 8) & 0xFF);
          wavFile.push_back((sampleRate >> 16) & 0xFF); wavFile.push_back((sampleRate >> 24) & 0xFF);
          
          uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
          wavFile.push_back(byteRate & 0xFF); wavFile.push_back((byteRate >> 8) & 0xFF);
          wavFile.push_back((byteRate >> 16) & 0xFF); wavFile.push_back((byteRate >> 24) & 0xFF);
          
          uint16_t blockAlign = numChannels * (bitsPerSample / 8);
          wavFile.push_back(blockAlign & 0xFF); wavFile.push_back((blockAlign >> 8) & 0xFF);
          
          wavFile.push_back(bitsPerSample & 0xFF); wavFile.push_back((bitsPerSample >> 8) & 0xFF);
          
          wavFile.push_back('d'); wavFile.push_back('a'); wavFile.push_back('t'); wavFile.push_back('a');
          wavFile.push_back(dataSize & 0xFF); wavFile.push_back((dataSize >> 8) & 0xFF);
          wavFile.push_back((dataSize >> 16) & 0xFF); wavFile.push_back((dataSize >> 24) & 0xFF);
          
          wavFile.insert(wavFile.end(), pcmData.begin(), pcmData.end());
          
          return wavFile;
      }
    }
  private:
    std::string m_CurrentVoiceId;
  };

  std::unique_ptr<NativeAudioProviderImpl> CreateNativeAudioProviderImpl()
  {
    return std::make_unique<MacNativeAudioProviderImpl>();
  }
}
#endif
