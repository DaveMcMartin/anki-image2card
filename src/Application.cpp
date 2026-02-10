#include "Application.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <httplib.h>
#include <iostream>
#include <string>

#include "IconsFontAwesome6.h"
#include "ai/ElevenLabsAudioProvider.h"
#include "ai/GoogleTextProvider.h"
#include "ai/MiniMaxAudioProvider.h"
#include "ai/NativeAudioProvider.h"
#include "ai/XAiTextProvider.h"
#include "api/AnkiConnectClient.h"
#include "config/ConfigManager.h"
#include "core/Logger.h"
#include "core/sdl/SDLWrappers.h"
#include "language/JapaneseLanguage.h"
#include "language/analyzer/SentenceAnalyzer.h"
#include "language/audio/ForvoClient.h"
#include "language/services/AITranslationService.h"
#include "language/services/DeepLService.h"
#include "language/services/GoogleTranslateService.h"
#include "language/services/NoneTranslationService.h"
#include "ocr/NativeOCRProvider.h"
#include "ocr/TesseractOCRProvider.h"
#include "stb_image.h"
#include "ui/AnkiCardSettingsSection.h"
#include "ui/ConfigurationSection.h"
#include "ui/ImageSection.h"
#include "ui/StatusSection.h"
#include "utils/ImageProcessor.h"

namespace Image2Card
{

  Application::Application(std::string title, int width, int height)
      : m_Title(std::move(title))
      , m_Width(width)
      , m_Height(height)
      , m_IsRunning(false)
      , m_Window(nullptr)
      , m_Renderer(nullptr)
      , m_BasePath("")
  {}

  Application::~Application()
  {
    Shutdown();
  }

  void Application::Run()
  {
    if (!Initialize()) {
      return;
    }

    m_IsRunning = true;
    while (m_IsRunning) {
      HandleEvents();
      Update();
      Render();
    }
  }

  bool Application::Initialize()
  {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
      AF_ERROR("Error: SDL_Init(): {}", SDL_GetError());
      return false;
    }

    const char* base = SDL_GetBasePath();
    if (base) {
      m_BasePath = std::string(base);
    } else {
      AF_ERROR("SDL_GetBasePath failed: {}", SDL_GetError());
    }

    auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN |
                                           SDL_WINDOW_HIGH_PIXEL_DENSITY);
    m_Window = SDL_CreateWindow(m_Title.c_str(), m_Width, m_Height, window_flags);
    if (m_Window == nullptr) {
      AF_ERROR("Error: SDL_CreateWindow(): {}", SDL_GetError());
      return false;
    }

    m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
    if (m_Renderer == nullptr) {
      AF_ERROR("Error: SDL_CreateRenderer(): {}", SDL_GetError());
      return false;
    }

    SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    {
      const std::string iconPath = m_BasePath + "assets/logo.png";
      int iconWidth{}, iconHeight{}, iconChannels{};
      unsigned char* iconData = stbi_load(iconPath.c_str(), &iconWidth, &iconHeight, &iconChannels, 4);
      if (iconData) {
        auto iconSurface = SDL::MakeSurfaceFrom(iconWidth, iconHeight, SDL_PIXELFORMAT_RGBA32, iconData, iconWidth * 4);

        if (iconSurface) {
          SDL_SetWindowIcon(m_Window, iconSurface.get());
        } else {
          AF_WARN("Failed to create icon surface: {}", SDL_GetError());
        }
        stbi_image_free(iconData);
      } else {
        AF_WARN("Failed to load {} for window icon.", iconPath);
      }
    }

    SDL_ShowWindow(m_Window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.3f;
    style.FrameRounding = 2.3f;
    style.ScrollbarRounding = 0;

    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.01f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.87f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.48f, 0.72f, 0.89f, 0.49f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    ImGui_ImplSDL3_InitForSDLRenderer(m_Window, m_Renderer);
    ImGui_ImplSDLRenderer3_Init(m_Renderer);

    std::string fontPath = m_BasePath + "assets/NotoSansJP-Regular.otf";
    float fontSize = 24.0f;
    ImFont* font =
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, nullptr, io.Fonts->GetGlyphRangesJapanese());
    if (font == nullptr) {
      AF_WARN("Warning: Could not load font {}. Using default font.", fontPath);
    }

    float iconFontSize = fontSize * 2.0f / 3.0f;
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;

    std::string iconFontPath = m_BasePath + "assets/fa-solid-900.ttf";
    io.Fonts->AddFontFromFileTTF(iconFontPath.c_str(), iconFontSize, &icons_config, icons_ranges);

    char* prefPath = SDL_GetPrefPath("Image2Card", "AnkiImage2Card");
    std::string configPath = "config.json";
    if (prefPath) {
      configPath = std::string(prefPath) + "config.json";
      SDL_free(prefPath);
    }
    m_ConfigManager = std::make_unique<Config::ConfigManager>(configPath);

    m_Languages.push_back(std::make_unique<Language::JapaneseLanguage>());
    std::string selectedLang = m_ConfigManager->GetConfig().SelectedLanguage;
    for (auto& lang : m_Languages) {
      if (lang->GetIdentifier() == selectedLang) {
        m_ActiveLanguage = lang.get();
        break;
      }
    }
    if (!m_ActiveLanguage && !m_Languages.empty()) {
      m_ActiveLanguage = m_Languages[0].get();
    }

    m_TextAIProviders.push_back(std::make_shared<AI::GoogleTextProvider>());
    m_TextAIProviders.push_back(std::make_shared<AI::XAiTextProvider>());

    for (auto& provider : m_TextAIProviders) {
      nlohmann::json providerConfig;
      if (provider->GetId() == "xai") {
        providerConfig["api_key"] = m_ConfigManager->GetConfig().TextApiKey;
        providerConfig["available_models"] = m_ConfigManager->GetConfig().TextAvailableModels;
      } else if (provider->GetId() == "google") {
        providerConfig["api_key"] = m_ConfigManager->GetConfig().GoogleApiKey;
        providerConfig["available_models"] = m_ConfigManager->GetConfig().GoogleAvailableModels;
      }
      provider->LoadConfig(providerConfig);
    }

    auto& config = m_ConfigManager->GetConfig();
    if (config.AudioProvider == "native") {
      m_AudioAIProvider = std::make_unique<AI::NativeAudioProvider>();
    } else if (config.AudioProvider == "minimax") {
      m_AudioAIProvider = std::make_unique<AI::MiniMaxAudioProvider>();
    } else {
      m_AudioAIProvider = std::make_unique<AI::ElevenLabsAudioProvider>();
    }

    m_TesseractOCRProvider = std::make_unique<OCR::TesseractOCRProvider>();
    std::string tessDataPath = m_BasePath + "tessdata";
    std::string tessLanguage = "jpn";
    if (!m_TesseractOCRProvider->Initialize(tessDataPath, tessLanguage)) {
      AF_WARN("Failed to initialize Tesseract OCR. AI OCR will be used as fallback.");
    }

    m_NativeOCRProvider = std::make_unique<OCR::NativeOCRProvider>();
    if (m_NativeOCRProvider->IsInitialized()) {
      AF_INFO("Native OCR provider initialized successfully");
    } else {
      AF_WARN("Native OCR provider not available on this platform");
    }

    {
      nlohmann::json audioConfig;
      auto& config = m_ConfigManager->GetConfig();

      if (config.AudioProvider == "minimax") {
        audioConfig["api_key"] = config.MiniMaxApiKey;
        audioConfig["voice_id"] = config.MiniMaxVoiceId;
        audioConfig["model"] = config.MiniMaxModel;

        nlohmann::json voicesJson = nlohmann::json::array();
        for (const auto& voice : config.MiniMaxAvailableVoices) {
          voicesJson.push_back({voice.first, voice.second});
        }
        audioConfig["available_voices"] = voicesJson;
      } else {
        audioConfig["api_key"] = config.ElevenLabsApiKey;
        audioConfig["voice_id"] = config.ElevenLabsVoiceId;

        nlohmann::json voicesJson = nlohmann::json::array();
        for (const auto& voice : config.ElevenLabsAvailableVoices) {
          voicesJson.push_back({voice.first, voice.second});
        }
        audioConfig["available_voices"] = voicesJson;
      }

      m_AudioAIProvider->LoadConfig(audioConfig);
    }

    if (m_ConfigManager->GetConfig().SelectedVoiceModel.empty()) {
      auto& config = m_ConfigManager->GetConfig();
      if (config.AudioProvider == "minimax") {
        config.SelectedVoiceModel = "MiniMax/" + config.MiniMaxVoiceId;
      } else {
        config.SelectedVoiceModel = "ElevenLabs/" + config.ElevenLabsVoiceId;
      }
    }

    auto noneService = std::make_unique<Language::Services::NoneTranslationService>();
    m_LanguageServices.push_back(std::move(noneService));

    auto deeplService = std::make_unique<Language::Services::DeepLService>();
    nlohmann::json deeplConfig;
    deeplConfig["api_key"] = m_ConfigManager->GetConfig().DeepLApiKey;
    deeplConfig["use_free_api"] = m_ConfigManager->GetConfig().DeepLUseFreeAPI;
    deeplConfig["source_lang"] = m_ConfigManager->GetConfig().DeepLSourceLang;
    deeplConfig["target_lang"] = m_ConfigManager->GetConfig().DeepLTargetLang;
    deeplService->LoadConfig(deeplConfig);
    m_LanguageServices.push_back(std::move(deeplService));

    auto googleTranslateService = std::make_unique<Language::Services::GoogleTranslateService>();
    nlohmann::json googleConfig;
    googleConfig["source_lang"] = "ja";
    googleConfig["target_lang"] = "en";
    googleTranslateService->LoadConfig(googleConfig);
    m_LanguageServices.push_back(std::move(googleTranslateService));

    for (auto& provider : m_TextAIProviders) {
      auto aiTranslationService =
          std::make_unique<Language::Services::AITranslationService>(provider, *m_ActiveLanguage);
      m_LanguageServices.push_back(std::move(aiTranslationService));
    }

    AF_INFO("Language services initialized");

    m_SentenceAnalyzer = std::make_unique<Language::Analyzer::SentenceAnalyzer>();
    m_SentenceAnalyzer->SetLanguageServices(&m_LanguageServices);

    std::string selectedTranslator = m_ConfigManager->GetConfig().SelectedTranslator;
    if (selectedTranslator.empty()) {
      selectedTranslator = "none";
    }
    m_SentenceAnalyzer->SetPreferredTranslator(selectedTranslator);

    if (m_SentenceAnalyzer->Initialize(m_BasePath)) {
      AF_INFO("Sentence analyzer initialized successfully");
    } else {
      AF_ERROR("Failed to initialize sentence analyzer");
    }

    m_ForvoClient = std::make_unique<Language::Audio::ForvoClient>("ja", 10, 1);
    AF_INFO("Forvo audio client initialized");

    std::string ankiUrl = m_ConfigManager->GetConfig().AnkiConnectUrl;
    if (ankiUrl.empty())
      ankiUrl = "http://localhost:8765";
    m_AnkiConnectClient = std::make_unique<API::AnkiConnectClient>(ankiUrl);

    m_ImageSection =
        std::make_unique<UI::ImageSection>(m_Renderer, &m_Languages, &m_ActiveLanguage, m_ConfigManager.get());
    m_ConfigurationSection = std::make_unique<UI::ConfigurationSection>(m_AnkiConnectClient.get(),
                                                                        m_ConfigManager.get(),
                                                                        &m_TextAIProviders,
                                                                        m_AudioAIProvider.get(),
                                                                        &m_LanguageServices,
                                                                        &m_Languages,
                                                                        &m_ActiveLanguage);

    m_ConfigurationSection->SetOnTranslatorChangedCallback([this](const std::string& translatorId) {
      if (m_SentenceAnalyzer) {
        m_SentenceAnalyzer->SetPreferredTranslator(translatorId);
      }
    });

    m_ConfigurationSection->SetOnAudioProviderChangedCallback([this](const std::string& providerId) {
      if (providerId == "native") {
        m_AudioAIProvider = std::make_unique<AI::NativeAudioProvider>();
      } else if (providerId == "minimax") {
        m_AudioAIProvider = std::make_unique<AI::MiniMaxAudioProvider>();
      } else {
        m_AudioAIProvider = std::make_unique<AI::ElevenLabsAudioProvider>();
      }

      nlohmann::json audioConfig;
      auto& config = m_ConfigManager->GetConfig();

      if (providerId == "minimax") {
        audioConfig["api_key"] = config.MiniMaxApiKey;
        audioConfig["voice_id"] = config.MiniMaxVoiceId;
        audioConfig["model"] = config.MiniMaxModel;

        nlohmann::json voicesJson = nlohmann::json::array();
        for (const auto& voice : config.MiniMaxAvailableVoices) {
          voicesJson.push_back({voice.first, voice.second});
        }
        audioConfig["available_voices"] = voicesJson;
      } else if (providerId == "elevenlabs") {
        audioConfig["api_key"] = config.ElevenLabsApiKey;
        audioConfig["voice_id"] = config.ElevenLabsVoiceId;

        nlohmann::json voicesJson = nlohmann::json::array();
        for (const auto& voice : config.ElevenLabsAvailableVoices) {
          voicesJson.push_back({voice.first, voice.second});
        }
        audioConfig["available_voices"] = voicesJson;
      }

      m_AudioAIProvider->LoadConfig(audioConfig);
      m_ConfigurationSection->SetAudioProvider(m_AudioAIProvider.get());

      std::thread([this]() { m_AudioAIProvider->LoadRemoteVoices(); }).detach();
    });

    m_AnkiCardSettingsSection =
        std::make_unique<UI::AnkiCardSettingsSection>(m_Renderer, m_AnkiConnectClient.get(), m_ConfigManager.get());

    m_ConfigurationSection->SetOnNoteTypeOrDeckChangedCallback([this]() {
      if (m_AnkiCardSettingsSection) {
        m_AnkiCardSettingsSection->RefreshData();
      }
    });
    m_StatusSection = std::make_unique<UI::StatusSection>();

    m_AnkiCardSettingsSection->SetOnStatusMessageCallback([this](const std::string& msg) {
      if (m_StatusSection)
        m_StatusSection->SetStatus(msg);
    });

    m_ConfigurationSection->SetOnConnectCallback([this]() {
      if (m_AnkiCardSettingsSection) {
        m_AnkiCardSettingsSection->RefreshData();
      }
      m_AnkiConnected.store(true);
      if (m_StatusSection)
        m_StatusSection->SetStatus("AnkiConnect: Connected");
    });

    m_ImageSection->SetOnScanCallback([this]() { OnScan(); });

    std::thread([this]() {
      if (m_AnkiConnectClient && m_AnkiConnectClient->Ping()) {
        m_AnkiConnected.store(true);
        if (m_StatusSection)
          m_StatusSection->SetStatus("AnkiConnect: Connected");
        if (m_AnkiCardSettingsSection) {
          m_AnkiCardSettingsSection->RefreshData();
        }
      } else {
        m_AnkiConnected.store(false);
        if (m_StatusSection)
          m_StatusSection->SetStatus("AnkiConnect: Not connected (click Connect to retry)");
      }
    }).detach();

    return true;
  }

  void Application::Shutdown()
  {
    CancelAsyncTasks();

    m_StatusSection.reset();
    m_AnkiCardSettingsSection.reset();
    m_ConfigurationSection.reset();
    m_ImageSection.reset();

    m_AudioAIProvider.reset();
    m_TextAIProviders.clear();
    m_Languages.clear();
    m_ConfigManager.reset();
    m_AnkiConnectClient.reset();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (m_Renderer) {
      SDL_DestroyRenderer(m_Renderer);
      m_Renderer = nullptr;
    }

    if (m_Window) {
      SDL_DestroyWindow(m_Window);
      m_Window = nullptr;
    }

    // Note: SDL_Quit() is intentionally skipped to avoid double-free crashes.
    // SDL 3.0 will clean up resources automatically when the process exits.
    // Calling SDL_Quit() after we've already destroyed textures and surfaces
    // causes malloc errors due to double-free attempts.
  }

  void Application::HandleEvents()
  {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);

      if (event.type == SDL_EVENT_QUIT) {
        m_IsRunning = false;
      }

      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window)) {
        m_IsRunning = false;
      }

      if (event.type == SDL_EVENT_DROP_FILE) {
        if (m_ImageSection && event.drop.data) {
          m_ImageSection->LoadImageFromFile(event.drop.data);
          // SDL3: event.drop.data is owned by the event, do not free.
        }
      }
    }
  }

  void Application::Update()
  {
    UpdateAsyncTasks();
  }

  void Application::Render()
  {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    RenderUI();

    ImGui::Render();
    SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_Renderer);

    ImGuiIO& io = ImGui::GetIO();
    SDL_SetRenderScale(m_Renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_Renderer);
    SDL_RenderPresent(m_Renderer);
  }

  void Application::RenderUI()
  {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                    ImGuiWindowFlags_NoBackground;

    ImGui::Begin("MainDockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    static bool first_time = true;
    if (first_time) {
      first_time = false;

      ImGui::DockBuilderRemoveNode(dockspace_id);
      ImGui::DockBuilderAddNode(dockspace_id,
                                ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_NoWindowMenuButton |
                                    ImGuiDockNodeFlags_NoCloseButton);
      ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

      ImGuiID dock_main_id = dockspace_id;
      ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.10f, nullptr, &dock_main_id);
      ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.5f, nullptr, &dock_main_id);
      ImGuiID dock_right_id = dock_main_id;

      ImGui::DockBuilderDockWindow("Image Section", dock_left_id);
      ImGui::DockBuilderDockWindow("RightPanel", dock_right_id);
      ImGui::DockBuilderDockWindow("Status", dock_bottom_id);

      ImGuiDockNode* node;
      if ((node = ImGui::DockBuilderGetNode(dock_left_id)))
        node->LocalFlags |=
            ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton;
      if ((node = ImGui::DockBuilderGetNode(dock_right_id)))
        node->LocalFlags |=
            ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton;
      if ((node = ImGui::DockBuilderGetNode(dock_bottom_id)))
        node->LocalFlags |=
            ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton;

      ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::End();

    if (m_ImageSection) {
      m_ImageSection->Render();
    }

    ImGui::Begin("RightPanel", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
    if (ImGui::BeginTabBar("RightPanelTabs")) {
      if (ImGui::BeginTabItem("Card")) {
        if (m_AnkiCardSettingsSection) {
          m_AnkiCardSettingsSection->Render();
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("AnkiConnect")) {
        if (m_ConfigurationSection) {
          m_ConfigurationSection->RenderAnkiConnectTab();
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("OCR")) {
        if (m_ConfigurationSection) {
          m_ConfigurationSection->RenderOCRTab();
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Dictionary")) {
        if (m_ConfigurationSection) {
          m_ConfigurationSection->RenderDictionaryTab();
        }
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Settings")) {
        if (m_ConfigurationSection) {
          m_ConfigurationSection->RenderConfigurationTab();
        }
        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }
    ImGui::End();

    if (m_StatusSection) {
      m_StatusSection->Render();
    }

    RenderScanModal();
  }

  AI::ITextAIProvider* Application::GetTextProviderForModel(const std::string& modelLabel)
  {
    if (modelLabel.empty()) {
      if (!m_TextAIProviders.empty())
        return m_TextAIProviders[0].get();
      return nullptr;
    }

    std::string providerName;
    size_t slashPos = modelLabel.find('/');
    if (slashPos != std::string::npos) {
      providerName = modelLabel.substr(0, slashPos);
    }

    for (auto& provider : m_TextAIProviders) {
      if (providerName == "xAI" && provider->GetId() == "xai")
        return provider.get();
      if (providerName == "Google" && provider->GetId() == "google")
        return provider.get();
    }

    if (!m_TextAIProviders.empty())
      return m_TextAIProviders[0].get();
    return nullptr;
  }

  void Application::OnScan()
  {
    if (m_IsScanning.load()) {
      AF_WARN("Scan already in progress, ignoring request.");
      if (m_StatusSection)
        m_StatusSection->SetStatus("Scan already in progress.");
      return;
    }

    AF_INFO("Starting Scan...");
    if (m_StatusSection)
      m_StatusSection->SetStatus("Scanning image...");

    {
      std::lock_guard<std::mutex> lock(m_ResultMutex);
      m_LastError.clear();
      m_OCRResult.clear();
    }

    if (!m_AnkiConnected.load()) {
      if (m_StatusSection)
        m_StatusSection->SetStatus("Error: Anki is not connected.");
      AF_ERROR("Anki is not connected.");
      return;
    }

    std::vector<unsigned char> imageBytes = m_ImageSection->GetSelectedImageBytes();
    AF_INFO("Image selected, size: {} bytes", imageBytes.size());

    if (imageBytes.empty()) {
      if (m_StatusSection)
        m_StatusSection->SetStatus("Error: No image selected.");
      AF_ERROR("No image selected.");
      return;
    }

    m_IsScanning.store(true);
    m_OCRComplete = false;

    AF_INFO("Launching async OCR task...");
    if (m_StatusSection)
      m_StatusSection->SetProgress(0.0f);

    std::vector<unsigned char> scanImage = imageBytes;

    auto& config = m_ConfigManager->GetConfig();
    std::string ocrMethod = config.OCRMethod;
    std::string tesseractOrientation = m_ImageSection->GetTesseractOrientation();
    std::string selectedVisionModel = config.SelectedVisionModel;

    AsyncTask task;
    task.description = "OCR Image Processing";
    task.future = std::async(
        std::launch::async,
        [this, imageBytes = std::move(imageBytes), ocrMethod, tesseractOrientation, selectedVisionModel]() {
          try {
            if (m_CancelRequested.load()) {
              AF_INFO("OCR task cancelled before starting.");
              return;
            }

            std::string text;

            if (ocrMethod == "Native" && m_NativeOCRProvider && m_NativeOCRProvider->IsInitialized()) {
              AF_INFO("Using Native OS OCR");
              text = m_NativeOCRProvider->ExtractTextFromImage(imageBytes);
            } else if (ocrMethod == "Tesseract" && m_TesseractOCRProvider && m_TesseractOCRProvider->IsInitialized()) {
              AF_INFO("Using Tesseract OCR with orientation: {}", tesseractOrientation);

              if (tesseractOrientation == "vertical") {
                m_TesseractOCRProvider->SetOrientation(OCR::TesseractOrientation::Vertical);
              } else {
                m_TesseractOCRProvider->SetOrientation(OCR::TesseractOrientation::Horizontal);
              }

              text = m_TesseractOCRProvider->ExtractTextFromImage(imageBytes);
            } else {
              AF_INFO("Sending image to Text AI Provider for OCR...");
              auto* provider = GetTextProviderForModel(selectedVisionModel);
              if (!provider) {
                throw std::runtime_error("No Text AI Provider found for selected vision model.");
              }

              std::string modelName = selectedVisionModel;
              size_t slashPos = modelName.find('/');
              if (slashPos != std::string::npos) {
                modelName = modelName.substr(slashPos + 1);
              }
              nlohmann::json config;
              config["vision_model"] = modelName;
              provider->LoadConfig(config);

              text = provider->ExtractTextFromImage(imageBytes, "image/png", *m_ActiveLanguage);
            }

            if (m_ActiveLanguage) {
              text = m_ActiveLanguage->PostProcessOCR(text);
            }

            AF_INFO("OCR Result: {}", text);

            {
              std::lock_guard<std::mutex> lock(m_ResultMutex);
              m_OCRResult = std::move(text);
              m_OCRComplete = true;
            }
          } catch (const std::exception& e) {
            AF_ERROR("OCR task failed with exception: {}", e.what());
            std::lock_guard<std::mutex> lock(m_ResultMutex);
            m_LastError = std::string("OCR failed: ") + e.what();
          } catch (...) {
            AF_ERROR("OCR task failed with unknown exception.");
            std::lock_guard<std::mutex> lock(m_ResultMutex);
            m_LastError = "OCR failed with unknown error.";
          }
        });

    task.onComplete = [this, scanImage = std::move(scanImage)]() {
      m_IsScanning.store(false);
      if (m_StatusSection)
        m_StatusSection->SetProgress(-1.0f);

      std::string ocrResult;
      std::string error;
      {
        std::lock_guard<std::mutex> lock(m_ResultMutex);
        ocrResult = m_OCRResult;
        error = m_LastError;
      }

      if (!error.empty()) {
        if (m_StatusSection)
          m_StatusSection->SetStatus("Error: " + error);
        AF_ERROR("OCR failed: {}", error);
        return;
      }

      if (ocrResult.empty()) {
        if (m_StatusSection)
          m_StatusSection->SetStatus("Error: OCR returned no text.");
        AF_ERROR("OCR returned no text.");
        return;
      }

      m_ScanSentence = ocrResult;
      m_ScanTargetWord = "";

      auto& config = m_ConfigManager->GetConfig();
      if (config.AudioProvider == "minimax") {
        m_ScanVoice = config.MiniMaxVoiceId;
      } else {
        m_ScanVoice = config.ElevenLabsVoiceId;
      }

      m_ScanSentence.reserve(256);
      m_ScanTargetWord.reserve(64);
      m_ScanVoice.reserve(64);

      if (m_StatusSection)
        m_StatusSection->SetStatus("Scan complete.");
      AF_INFO("Scan complete.");

      if (m_AnkiCardSettingsSection) {
        m_AnkiCardSettingsSection->SetFieldByTool(7, scanImage, "image.png");
      }

      m_ShowScanModal = true;
      m_OpenScanModal = true;
    };

    task.onError = [this](const std::string& error) {
      m_IsScanning.store(false);
      if (m_StatusSection) {
        m_StatusSection->SetStatus("Error: " + error);
        m_StatusSection->SetProgress(-1.0f);
      }
      AF_ERROR("Scan error: {}", error);
    };

    {
      std::lock_guard<std::mutex> lock(m_TaskMutex);
      m_ActiveTasks.push(std::move(task));
    }
  }

  void Application::RenderScanModal()
  {
    if (m_OpenScanModal) {
      ImGui::OpenPopup("Scan Result");
      m_OpenScanModal = false;
    }

    if (ImGui::BeginPopupModal("Scan Result", &m_ShowScanModal, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::PushItemWidth(400.0f);

      auto InputText = [](const char* label, std::string* str) {
        auto callback = [](ImGuiInputTextCallbackData* data) {
          if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            std::string* s = (std::string*) data->UserData;
            IM_ASSERT(data->Buf == s->c_str());
            s->resize(data->BufTextLen);
            data->Buf = (char*) s->data();
          }
          return 0;
        };
        return ImGui::InputText(
            label, (char*) str->data(), str->capacity() + 1, ImGuiInputTextFlags_CallbackResize, callback, (void*) str);
      };
      auto InputTextMultiline = [](const char* label, std::string* str) {
        auto callback = [](ImGuiInputTextCallbackData* data) {
          if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            std::string* s = (std::string*) data->UserData;
            IM_ASSERT(data->Buf == s->c_str());
            s->resize(data->BufTextLen);
            data->Buf = (char*) s->data();
          }
          return 0;
        };
        return ImGui::InputTextMultiline(label,
                                         (char*) str->data(),
                                         str->capacity() + 1,
                                         ImVec2(0, 120),
                                         ImGuiInputTextFlags_CallbackResize,
                                         callback,
                                         (void*) str);
      };

      InputTextMultiline("Sentence", &m_ScanSentence);
      InputText("Target Word", &m_ScanTargetWord);

      if (m_AudioAIProvider) {
        if (m_AudioAIProvider->RenderVoiceSelector("Voice", &m_ScanVoice)) {
          m_AudioAIProvider->SetVoiceId(m_ScanVoice);
          auto& config = m_ConfigManager->GetConfig();
          if (config.AudioProvider == "minimax") {
            config.MiniMaxVoiceId = m_ScanVoice;
          } else if (config.AudioProvider == "elevenlabs") {
            config.ElevenLabsVoiceId = m_ScanVoice;
          }
          m_ConfigManager->Save();
        }
      }

      ImGui::PopItemWidth();
      ImGui::Separator();

      bool isProcessing = m_IsProcessing.load();
      if (isProcessing) {
        ImGui::BeginDisabled();
      }

      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.60f, 0.20f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.75f, 0.25f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.50f, 0.15f, 1.0f));

      if (ImGui::Button("Process", ImVec2(120, 0))) {
        ProcessScan();
        m_ShowScanModal = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::PopStyleColor(3);

      if (isProcessing) {
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Processing...");
      } else {
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.15f, 0.15f, 1.0f));

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          m_ShowScanModal = false;
          ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleColor(3);
      }

      ImGui::EndPopup();
    }
  }

  void Application::ProcessScan()
  {
    // Prevent multiple simultaneous processing
    if (m_IsProcessing.load()) {
      AF_WARN("Processing already in progress, ignoring request.");
      if (m_StatusSection)
        m_StatusSection->SetStatus("Processing already in progress.");
      return;
    }

    AF_INFO("Processing Scan. Sentence: '{}', Target Word: '{}'", m_ScanSentence, m_ScanTargetWord);
    if (m_StatusSection)
      m_StatusSection->SetStatus("Processing scan...");

    // Capture needed data for async task
    std::string sentence = m_ScanSentence;
    std::string targetWord = m_ScanTargetWord;
    std::string voice = m_ScanVoice;

    std::vector<unsigned char> fullImage;
    if (m_ImageSection) {
      fullImage = m_ImageSection->GetFullImageBytes();
    }

    m_IsProcessing.store(true);

    // Launch processing task in background thread
    AF_INFO("Launching async processing task...");
    if (m_StatusSection)
      m_StatusSection->SetProgress(0.1f);

    std::string languageCode = m_ActiveLanguage ? m_ActiveLanguage->GetLanguageCode() : "";
    std::string selectedAnalysisModel = m_ConfigManager->GetConfig().SelectedAnalysisModel;

    {
      std::lock_guard<std::mutex> lock(m_ResultMutex);
      m_LastError.clear();
    }

    AsyncTask task;
    task.description = "Scan Processing";
    task.future = std::async(
        std::launch::async, [this, sentence, targetWord, voice, fullImage, languageCode, selectedAnalysisModel]() {
          try {
            if (m_CancelRequested.load()) {
              AF_INFO("Processing task cancelled before starting.");
              return;
            }
            AF_INFO("Analyzing sentence...");
            AF_DEBUG("Sentence: '{}', Target Word: '{}'", sentence, targetWord);

            if (!m_ActiveLanguage) {
              throw std::runtime_error("No active language selected");
            }

            nlohmann::json analysis;

            if (m_SentenceAnalyzer && m_SentenceAnalyzer->IsReady()) {
              AF_INFO("Using local sentence analyzer");
              analysis = m_SentenceAnalyzer->AnalyzeSentence(sentence, targetWord, m_ActiveLanguage);
            } else {
              AF_INFO("Using AI for sentence analysis");
              auto* provider = GetTextProviderForModel(selectedAnalysisModel);
              if (!provider) {
                throw std::runtime_error("No Text AI Provider found for selected analysis model.");
              }

              // Update provider with selected model
              std::string modelName = selectedAnalysisModel;
              size_t slashPos = modelName.find('/');
              if (slashPos != std::string::npos) {
                modelName = modelName.substr(slashPos + 1);
              }
              nlohmann::json config;
              config["sentence_model"] = modelName;
              provider->LoadConfig(config);

              analysis = provider->AnalyzeSentence(sentence, targetWord, *m_ActiveLanguage);
            }

            if (m_CancelRequested.load()) {
              AF_INFO("Processing task cancelled after analysis.");
              return;
            }

            AF_DEBUG("Analysis Response: {}", analysis.dump());
            if (analysis.is_null()) {
              AF_ERROR("Analysis returned null/empty response");
              throw std::runtime_error("Text analysis failed.");
            }

            if (m_StatusSection)
              m_StatusSection->SetProgress(0.4f);

            AF_INFO("Analysis Result: {}", analysis.dump());

            std::string analyzedSentence = analysis.value("sentence", "");
            std::string translation = analysis.value("translation", "");
            std::string analyzedTargetWord = analysis.value("target_word", "");
            std::string targetWordFurigana = analysis.value("target_word_furigana", "");
            std::string furigana = analysis.value("furigana", "");
            std::string definition = analysis.value("definition", "");
            std::string pitch = analysis.value("pitch_accent", "");

            auto updateFields = [this,
                                 analyzedSentence,
                                 translation,
                                 analyzedTargetWord,
                                 targetWordFurigana,
                                 furigana,
                                 definition,
                                 pitch,
                                 fullImage]() {
              if (m_AnkiCardSettingsSection) {
                AF_INFO("Setting fields in Anki Card Settings...");
                m_AnkiCardSettingsSection->SetFieldByTool(0, analyzedSentence);
                m_AnkiCardSettingsSection->SetFieldByTool(1, furigana);
                m_AnkiCardSettingsSection->SetFieldByTool(2, translation);
                m_AnkiCardSettingsSection->SetFieldByTool(3, analyzedTargetWord);
                m_AnkiCardSettingsSection->SetFieldByTool(4, targetWordFurigana);
                m_AnkiCardSettingsSection->SetFieldByTool(5, pitch);
                m_AnkiCardSettingsSection->SetFieldByTool(6, definition);
                if (!fullImage.empty()) {
                  m_AnkiCardSettingsSection->SetFieldByTool(7, fullImage, "image.png");
                }
              } else {
                AF_WARN("AnkiCardSettingsSection is null, cannot set fields.");
              }
            };

            updateFields();
            if (!analyzedTargetWord.empty() && !m_CancelRequested.load()) {
              if (m_StatusSection)
                m_StatusSection->SetProgress(0.6f);
              AF_INFO("Generating Vocab Audio for: {}", analyzedTargetWord);
              if (m_StatusSection)
                m_StatusSection->SetStatus("Generating Vocab Audio...");

              std::vector<unsigned char> vocabAudio;

              if (m_ForvoClient && m_ForvoClient->IsAvailable()) {
                AF_INFO("Searching audio from Forvo");
                try {
                  auto audioResults = m_ForvoClient->SearchAudio(analyzedTargetWord, analyzedTargetWord, "");

                  if (!audioResults.empty()) {
                    std::string audioUrl = audioResults[0].url;
                    if (audioUrl.find("https://") == 0) {
                      audioUrl = audioUrl.substr(8);
                    }

                    size_t slashPos = audioUrl.find('/');
                    if (slashPos != std::string::npos) {
                      std::string host = audioUrl.substr(0, slashPos);
                      std::string path = audioUrl.substr(slashPos);

                      httplib::SSLClient audioClient(host.c_str());
                      audioClient.set_connection_timeout(10, 0);
                      audioClient.set_read_timeout(10, 0);

                      auto res = audioClient.Get(path.c_str());
                      if (res && res->status == 200) {
                        vocabAudio.assign(res->body.begin(), res->body.end());
                        AF_INFO("Downloaded vocab audio from Forvo: {} ({} bytes)",
                                audioResults[0].filename,
                                vocabAudio.size());
                      } else {
                        AF_WARN("Failed to download vocab audio from: {}", audioUrl);
                      }
                    }
                  }
                } catch (const std::exception& e) {
                  AF_WARN("Forvo audio search failed: {}, falling back to AI", e.what());
                }
              }

              if (vocabAudio.empty() && m_AudioAIProvider) {
                AF_INFO("Using AI for vocab audio generation");
                std::string audioFormat = m_ConfigManager->GetConfig().AudioFormat;
                vocabAudio = m_AudioAIProvider->GenerateAudio(analyzedTargetWord, voice, languageCode, audioFormat);
                AF_INFO("Vocab Audio generated, size: {} bytes", vocabAudio.size());
              }

              if (m_AnkiCardSettingsSection && !vocabAudio.empty()) {
                // 8: Vocab Audio
                std::string filename = "vocab.mp3";
                if (m_ForvoClient && !vocabAudio.empty()) {
                  auto audioResults = m_ForvoClient->SearchAudio(analyzedTargetWord, analyzedTargetWord, "");
                  if (!audioResults.empty()) {
                    filename = audioResults[0].filename;
                  }
                } else {
                  std::string audioFormat = m_ConfigManager->GetConfig().AudioFormat;
                  std::string audioExt;
                  if (audioFormat == "opus") {
                    audioExt = (m_ConfigManager->GetConfig().AudioProvider == "minimax") ? "ogg" : "opus";
                  } else {
                    audioExt = "mp3";
                  }
                  filename = "vocab." + audioExt;
                }
                m_AnkiCardSettingsSection->SetFieldByTool(8, vocabAudio, filename);
              }
            }

            if (!analyzedSentence.empty() && !m_CancelRequested.load()) {
              if (m_StatusSection)
                m_StatusSection->SetProgress(0.8f);
              AF_INFO("Generating Sentence Audio for: {}", analyzedSentence);
              if (m_StatusSection)
                m_StatusSection->SetStatus("Generating Sentence Audio...");

              std::string audioFormat = m_ConfigManager->GetConfig().AudioFormat;
              std::vector<unsigned char> sentenceAudio =
                  m_AudioAIProvider->GenerateAudio(analyzedSentence, voice, languageCode, audioFormat);
              AF_INFO("Sentence Audio generated, size: {} bytes", sentenceAudio.size());

              if (m_AnkiCardSettingsSection && !sentenceAudio.empty()) {
                // 9: Sentence Audio
                std::string audioExt;
                if (audioFormat == "opus") {
                  audioExt = (m_ConfigManager->GetConfig().AudioProvider == "minimax") ? "ogg" : "opus";
                } else {
                  audioExt = "mp3";
                }
                m_AnkiCardSettingsSection->SetFieldByTool(9, sentenceAudio, "sentence." + audioExt);
              }
            }

            if (m_StatusSection)
              m_StatusSection->SetProgress(1.0f);
            AF_INFO("Processing complete.");
          } catch (const std::exception& e) {
            AF_ERROR("Processing task failed with exception: {}", e.what());
            std::lock_guard<std::mutex> lock(m_ResultMutex);
            m_LastError = std::string("Processing failed: ") + e.what();
          } catch (...) {
            AF_ERROR("Processing task failed with unknown exception.");
            std::lock_guard<std::mutex> lock(m_ResultMutex);
            m_LastError = "Processing failed with unknown error.";
          }
        });

    task.onComplete = [this]() {
      m_IsProcessing.store(false);

      std::string error;
      {
        std::lock_guard<std::mutex> lock(m_ResultMutex);
        error = m_LastError;
      }

      if (!error.empty()) {
        if (m_StatusSection)
          m_StatusSection->SetStatus("Error: " + error);
        AF_ERROR("Processing failed: {}", error);
        return;
      }

      if (m_StatusSection)
        m_StatusSection->SetStatus("Processing complete.");
      AF_INFO("All processing tasks completed successfully.");
    };

    task.onError = [this](const std::string& error) {
      m_IsProcessing.store(false);
      if (m_StatusSection) {
        m_StatusSection->SetStatus("Error: " + error);
        m_StatusSection->SetProgress(-1.0f);
      }
      AF_ERROR("Processing error: {}", error);
    };

    // Add task to queue
    {
      std::lock_guard<std::mutex> lock(m_TaskMutex);
      m_ActiveTasks.push(std::move(task));
    }
  }

  void Application::UpdateAsyncTasks()
  {
    std::lock_guard<std::mutex> lock(m_TaskMutex);

    // Process completed tasks
    while (!m_ActiveTasks.empty()) {
      auto& task = m_ActiveTasks.front();

      // Check if task is complete
      if (task.future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        AF_INFO("Async task completed: {}", task.description);

        // Get the result (this will also handle any exceptions)
        try {
          task.future.get();

          // Call completion callback
          if (task.onComplete) {
            task.onComplete();
          }
        } catch (const std::exception& e) {
          AF_ERROR("Async task '{}' threw exception: {}", task.description, e.what());
          if (task.onError) {
            task.onError(e.what());
          }
        } catch (...) {
          AF_ERROR("Async task '{}' threw unknown exception", task.description);
          if (task.onError) {
            task.onError("Unknown error");
          }
        }

        // Remove completed task
        m_ActiveTasks.pop();
      } else {
        // Task still running, keep it in queue and break
        // (we only process the first task to maintain order)
        break;
      }
    }
  }

  void Application::CancelAsyncTasks()
  {
    AF_INFO("Cancelling all async tasks...");
    m_CancelRequested.store(true);

    std::lock_guard<std::mutex> lock(m_TaskMutex);

    // Wait for all tasks to complete
    while (!m_ActiveTasks.empty()) {
      auto& task = m_ActiveTasks.front();

      AF_INFO("Waiting for task to complete: {}", task.description);

      // Wait with timeout
      if (task.future.wait_for(std::chrono::seconds(5)) == std::future_status::ready) {
        try {
          task.future.get();
        } catch (...) {
          // Ignore exceptions during shutdown
        }
      } else {
        AF_WARN("Task '{}' did not complete within timeout, may still be running", task.description);
      }

      m_ActiveTasks.pop();
    }

    m_IsScanning.store(false);
    m_IsProcessing.store(false);
    m_CancelRequested.store(false);

    AF_INFO("All async tasks cancelled/completed.");
  }

} // namespace Image2Card
