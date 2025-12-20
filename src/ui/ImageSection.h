#pragma once

#include <imgui.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ui/UIComponent.h"

struct SDL_Texture;
struct SDL_Renderer;
struct SDL_Surface;

namespace Image2Card::Language
{
  class ILanguage;
}

namespace Image2Card::Config
{
  class ConfigManager;
}

namespace Image2Card::UI
{

  struct ImageData
  {
    SDL_Texture* texture = nullptr;
    SDL_Surface* surface = nullptr;
    int width = 0;
    int height = 0;
  };

  class ImageSection : public UIComponent
  {
public:

    ImageSection(SDL_Renderer* renderer,
                 std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                 Language::ILanguage** activeLanguage,
                 Config::ConfigManager* configManager);
    ~ImageSection() override;

    void Render() override;
    void Update() override;

    void SetOnScanCallback(std::function<void()> callback) { m_OnScanCallback = callback; }

    void LoadImageFromFile(const std::string& path);
    std::vector<unsigned char> GetSelectedImageBytes();
    std::vector<unsigned char> GetFullImageBytes();

private:

    void ClearImages();
    void RemoveCurrentImage();
    void ClearSelection();

    SDL_Renderer* m_Renderer;

    std::vector<ImageData> m_Images;
    size_t m_CurrentImageIndex = 0;

    // Cropping State
    bool m_IsSelecting = false;
    ImVec2 m_SelectionStart = {0.0f, 0.0f};
    ImVec2 m_SelectionEnd = {0.0f, 0.0f};

    // Screen coordinates of the image for mapping mouse clicks
    ImVec2 m_ImageScreenPos = {0.0f, 0.0f};
    ImVec2 m_ImageScreenSize = {0.0f, 0.0f};

    std::function<void()> m_OnScanCallback;

    // Language System
    std::vector<std::unique_ptr<Language::ILanguage>>* m_Languages;
    Language::ILanguage** m_ActiveLanguage;
    Config::ConfigManager* m_ConfigManager;
  };

} // namespace Image2Card::UI
