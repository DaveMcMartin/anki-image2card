#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

struct SDL_Window;
struct SDL_Renderer;

namespace Image2Card::OCR
{
  class TesseractOCRProvider;
  class NativeOCRProvider;
} // namespace Image2Card::OCR

namespace Image2Card::Language::Analyzer
{
  class SentenceAnalyzer;
}

namespace Image2Card::Language::Audio
{
  class ForvoClient;
}

namespace Image2Card::Language::Services
{
  class ILanguageService;
}

namespace Image2Card::UI
{
  class ImageSection;
  class ConfigurationSection;
  class AnkiCardSettingsSection;
  class StatusSection;
} // namespace Image2Card::UI

namespace Image2Card::API
{
  class AnkiConnectClient;
}

namespace Image2Card::AI
{
  class ITextAIProvider;
  class IAudioAIProvider;
} // namespace Image2Card::AI

namespace Image2Card::Language
{
  class ILanguage;
}

namespace Image2Card::Config
{
  class ConfigManager;
}

namespace Image2Card
{

  class Application
  {
public:

    Application(std::string title, int width, int height);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Run();

private:

    bool Initialize();
    void Shutdown();

    void HandleEvents();
    void Update();
    void Render();
    void RenderUI();

    void OnScan();
    void RenderScanModal();
    void ProcessScan();

    void UpdateAsyncTasks();
    void CancelAsyncTasks();

    AI::ITextAIProvider* GetTextProviderForModel(const std::string& modelName);

    std::string m_Title;
    int m_Width;
    int m_Height;
    bool m_IsRunning;

    SDL_Window* m_Window;
    SDL_Renderer* m_Renderer;

    std::string m_BasePath;

    std::unique_ptr<UI::ImageSection> m_ImageSection;
    std::unique_ptr<UI::ConfigurationSection> m_ConfigurationSection;
    std::unique_ptr<UI::AnkiCardSettingsSection> m_AnkiCardSettingsSection;
    std::unique_ptr<UI::StatusSection> m_StatusSection;

    std::unique_ptr<API::AnkiConnectClient> m_AnkiConnectClient;
    std::unique_ptr<Config::ConfigManager> m_ConfigManager;

    std::vector<std::shared_ptr<AI::ITextAIProvider>> m_TextAIProviders;

    std::unique_ptr<AI::IAudioAIProvider> m_AudioAIProvider;

    std::unique_ptr<OCR::TesseractOCRProvider> m_TesseractOCRProvider;
    std::unique_ptr<OCR::NativeOCRProvider> m_NativeOCRProvider;

    std::vector<std::unique_ptr<Language::Services::ILanguageService>> m_LanguageServices;
    std::unique_ptr<Language::Analyzer::SentenceAnalyzer> m_SentenceAnalyzer;
    std::unique_ptr<Language::Audio::ForvoClient> m_ForvoClient;

    std::vector<std::unique_ptr<Language::ILanguage>> m_Languages;
    Language::ILanguage* m_ActiveLanguage = nullptr;

    bool m_ShowScanModal = false;
    bool m_OpenScanModal = false;
    std::string m_ScanSentence;
    std::string m_ScanTargetWord;
    std::string m_ScanVoice;

    struct AsyncTask
    {
      std::future<void> future;
      std::string description;
      std::function<void()> onComplete;
      std::function<void(const std::string&)> onError;
    };

    std::queue<AsyncTask> m_ActiveTasks;
    std::mutex m_TaskMutex;

    std::atomic<bool> m_IsScanning{false};
    std::atomic<bool> m_IsProcessing{false};
    std::atomic<bool> m_CancelRequested{false};
    std::atomic<bool> m_AnkiConnected{false};

    std::mutex m_ResultMutex;
    std::string m_OCRResult;
    bool m_OCRComplete = false;
    std::string m_LastError;
  };

} // namespace Image2Card
