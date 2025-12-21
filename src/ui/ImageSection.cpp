#include "ui/ImageSection.h"

#include "config/ConfigManager.h"
#include "language/ILanguage.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <SDL3/SDL.h>

#include <imgui.h>

#include "IconsFontAwesome6.h"
#include "core/Logger.h"
#include "core/sdl/SDLWrappers.h"
#include "stb_image_write.h"

namespace Image2Card::UI
{

  ImageSection::ImageSection(SDL_Renderer* renderer,
                             std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                             Language::ILanguage** activeLanguage,
                             Config::ConfigManager* configManager)
      : m_Renderer(renderer)
      , m_Languages(languages)
      , m_ActiveLanguage(activeLanguage)
      , m_ConfigManager(configManager)
  {
    // Load tesseract orientation from config
    if (m_ConfigManager) {
      m_TesseractOrientation = m_ConfigManager->GetConfig().TesseractOrientation;
    }
  }

  ImageSection::~ImageSection()
  {
    ClearImages();
  }

  void ImageSection::ClearImages()
  {
    for (auto& img : m_Images) {
      if (img.texture) {
        SDL_DestroyTexture(img.texture);
      }
      if (img.surface) {
        SDL_DestroySurface(img.surface);
      }
    }
    m_Images.clear();
    m_CurrentImageIndex = 0;
    ClearSelection();
  }

  void ImageSection::RemoveCurrentImage()
  {
    if (m_Images.empty())
      return;

    auto& img = m_Images[m_CurrentImageIndex];
    if (img.texture)
      SDL_DestroyTexture(img.texture);
    if (img.surface)
      SDL_DestroySurface(img.surface);

    m_Images.erase(m_Images.begin() + m_CurrentImageIndex);

    if (m_Images.empty()) {
      m_CurrentImageIndex = 0;
    } else {
      if (m_CurrentImageIndex >= m_Images.size()) {
        m_CurrentImageIndex = m_Images.size() - 1;
      }
    }
    ClearSelection();
  }

  void ImageSection::ClearSelection()
  {
    m_IsSelecting = false;
    m_SelectionStart = {0.0f, 0.0f};
    m_SelectionEnd = {0.0f, 0.0f};
  }

  void ImageSection::LoadImageFromFile(const std::string& path)
  {
    int width{}, height{}, channels{};
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
      AF_ERROR("Failed to load image: {}", path);
      return;
    }

    auto tempSurface = SDL::MakeSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, width * 4);
    if (!tempSurface) {
      AF_ERROR("Failed to create surface: {}", SDL_GetError());
      stbi_image_free(data);
      return;
    }

    SDL_Surface* surface = SDL_DuplicateSurface(tempSurface.get());
    stbi_image_free(data);

    if (!surface) {
      AF_ERROR("Failed to duplicate surface: {}", SDL_GetError());
      return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
    if (!texture) {
      AF_ERROR("Failed to create texture: {}", SDL_GetError());
      SDL_DestroySurface(surface);
    } else {
      ImageData newImage;
      newImage.surface = surface;
      newImage.texture = texture;
      newImage.width = width;
      newImage.height = height;

      m_Images.push_back(newImage);

      // If this is the first image, ensure index is 0
      if (m_Images.size() == 1) {
        m_CurrentImageIndex = 0;
      }

      ClearSelection();
    }
  }

  void ImageSection::Update() {}

  void ImageSection::Render()
  {
    ImGui::Begin("Image Section", nullptr, ImGuiWindowFlags_NoScrollbar);

    if (m_Languages && m_ActiveLanguage && !m_Languages->empty()) {
      ImGui::SetNextItemWidth(150);
      if (ImGui::BeginCombo("Language", (*m_ActiveLanguage)->GetName().c_str())) {
        for (auto& language : *m_Languages) {
          bool isSelected = (language.get() == *m_ActiveLanguage);
          if (ImGui::Selectable(language->GetName().c_str(), isSelected)) {
            *m_ActiveLanguage = language.get();
            if (m_ConfigManager) {
              auto& config = m_ConfigManager->GetConfig();
              config.SelectedLanguage = language->GetIdentifier();
              m_ConfigManager->Save();
            }
          }
          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
    }

    // Orientation buttons for Tesseract OCR
    if (m_ConfigManager) {
      auto& config = m_ConfigManager->GetConfig();
      if (config.OCRMethod == "Tesseract") {
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("OCR:");
        ImGui::SameLine();

        bool isHorizontal = (m_TesseractOrientation == "horizontal");
        bool isVertical = (m_TesseractOrientation == "vertical");

        if (isHorizontal) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
        }
        if (ImGui::Button(ICON_FA_ARROWS_LEFT_RIGHT, ImVec2(30, 0))) {
          m_TesseractOrientation = "horizontal";
          config.TesseractOrientation = "horizontal";
          m_ConfigManager->Save();
        }
        if (isHorizontal) {
          ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Horizontal text");
        }

        ImGui::SameLine();

        if (isVertical) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
        }
        if (ImGui::Button(ICON_FA_ARROWS_UP_DOWN, ImVec2(30, 0))) {
          m_TesseractOrientation = "vertical";
          config.TesseractOrientation = "vertical";
          m_ConfigManager->Save();
        }
        if (isVertical) {
          ImGui::PopStyleColor();
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Vertical text");
        }
      }
    }

    ImGui::Separator();
    ImGui::Spacing();

    float footerHeight = 40.0f;
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    availSize.y -= footerHeight;

    if (availSize.y < 10.0f)
      availSize.y = 10.0f;

    float startY = ImGui::GetCursorPosY();

    ImageData* currentImage = nullptr;
    if (!m_Images.empty() && m_CurrentImageIndex < m_Images.size()) {
      currentImage = &m_Images[m_CurrentImageIndex];
    }

    if (currentImage && currentImage->texture) {
      float aspect = (float) currentImage->width / (float) currentImage->height;
      float viewAspect = availSize.x / availSize.y;

      ImVec2 imageSize = availSize;
      if (aspect > viewAspect) {
        imageSize.y = availSize.x / aspect;
      } else {
        imageSize.x = availSize.y * aspect;
      }

      float cursorX = ImGui::GetCursorPosX() + (availSize.x - imageSize.x) * 0.5f;
      float cursorY = ImGui::GetCursorPosY() + (availSize.y - imageSize.y) * 0.5f;

      ImGui::SetCursorPosX(cursorX);
      ImGui::SetCursorPosY(cursorY);

      ImGui::Image((ImTextureID) currentImage->texture, imageSize);

      m_ImageScreenPos = ImVec2(cursorX, cursorY);
      m_ImageScreenSize = imageSize;

      ImGuiIO& io = ImGui::GetIO();
      ImVec2 mousePos = io.MousePos;
      bool isHovered = ImGui::IsItemHovered();

      if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_IsSelecting = true;
        m_SelectionStart = mousePos;
        m_SelectionEnd = mousePos;
      }

      if (m_IsSelecting && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        m_SelectionEnd = mousePos;

        if (m_SelectionEnd.x < m_ImageScreenPos.x)
          m_SelectionEnd.x = m_ImageScreenPos.x;
        if (m_SelectionEnd.y < m_ImageScreenPos.y)
          m_SelectionEnd.y = m_ImageScreenPos.y;
        if (m_SelectionEnd.x > m_ImageScreenPos.x + m_ImageScreenSize.x)
          m_SelectionEnd.x = m_ImageScreenPos.x + m_ImageScreenSize.x;
        if (m_SelectionEnd.y > m_ImageScreenPos.y + m_ImageScreenSize.y)
          m_SelectionEnd.y = m_ImageScreenPos.y + m_ImageScreenSize.y;
      } else if (m_IsSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_IsSelecting = false;
      }

      if (m_SelectionStart.x != m_SelectionEnd.x || m_SelectionStart.y != m_SelectionEnd.y) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(m_SelectionStart, m_SelectionEnd, IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);
        drawList->AddRectFilled(m_SelectionStart, m_SelectionEnd, IM_COL32(0, 255, 0, 50));
      }
    } else {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
      ImGui::BeginChild("DropZone", availSize, true);

      const char* text = "Drop Image Here";
      ImVec2 textSize = ImGui::CalcTextSize(text);

      ImGui::SetCursorPosX((availSize.x - textSize.x) * 0.5f);
      ImGui::SetCursorPosY((availSize.y - textSize.y) * 0.5f);
      ImGui::Text("%s", text);

      ImGui::EndChild();
      ImGui::PopStyleColor();
    }

    ImGui::SetCursorPosY(startY + availSize.y);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button(ICON_FA_TRASH " Clear", ImVec2(100, 0))) {
      ClearImages();
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.59f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.69f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.49f, 0.10f, 1.0f));
    if (ImGui::Button(ICON_FA_WAND_MAGIC_SPARKLES " Scan", ImVec2(100, 0))) {
      if (m_OnScanCallback) {
        m_OnScanCallback();
      } else {
        AF_WARN("Scan requested (no callback)");
      }
    }
    ImGui::PopStyleColor(3);

    if (m_Images.size() > 1) {
      std::string countText = std::to_string(m_CurrentImageIndex + 1) + "/" + std::to_string(m_Images.size());

      float textWidth = ImGui::CalcTextSize(countText.c_str()).x;
      float buttonWidth = 30.0f;
      float spacing = ImGui::GetStyle().ItemSpacing.x;
      float groupWidth = textWidth + spacing + (buttonWidth * 3) + (spacing * 3);

      ImGui::SameLine();

      float availX = ImGui::GetContentRegionAvail().x;
      float cursorX = ImGui::GetCursorPosX();

      if (availX > groupWidth) {
        ImGui::SetCursorPosX(cursorX + availX - groupWidth);
      }

      ImGui::AlignTextToFramePadding();
      ImGui::Text("%s", countText.c_str());
      ImGui::SameLine();

      if (ImGui::Button(ICON_FA_XMARK, ImVec2(buttonWidth, 0))) {
        RemoveCurrentImage();
      }
      ImGui::SameLine();

      if (ImGui::Button(ICON_FA_ARROW_LEFT, ImVec2(buttonWidth, 0))) {
        if (m_CurrentImageIndex > 0) {
          m_CurrentImageIndex--;
        } else {
          m_CurrentImageIndex = m_Images.size() - 1;
        }
        ClearSelection();
      }
      ImGui::SameLine();

      if (ImGui::Button(ICON_FA_ARROW_RIGHT, ImVec2(buttonWidth, 0))) {
        if (m_CurrentImageIndex < m_Images.size() - 1) {
          m_CurrentImageIndex++;
        } else {
          m_CurrentImageIndex = 0;
        }
        ClearSelection();
      }
    }

    ImGui::End();
  }

  static void StbiWriteFunc(void* context, void* data, int size)
  {
    auto* vec = static_cast<std::vector<unsigned char>*>(context);
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    vec->insert(vec->end(), bytes, bytes + size);
  }

  std::vector<unsigned char> ImageSection::GetSelectedImageBytes()
  {
    if (m_Images.empty() || m_CurrentImageIndex >= m_Images.size())
      return {};

    ImageData& currentImg = m_Images[m_CurrentImageIndex];
    if (!currentImg.surface)
      return {};

    float x1 = std::min(m_SelectionStart.x, m_SelectionEnd.x);
    float y1 = std::min(m_SelectionStart.y, m_SelectionEnd.y);
    float x2 = std::max(m_SelectionStart.x, m_SelectionEnd.x);
    float y2 = std::max(m_SelectionStart.y, m_SelectionEnd.y);

    if ((x2 - x1) < 1.0f || (y2 - y1) < 1.0f) {
      x1 = m_ImageScreenPos.x;
      y1 = m_ImageScreenPos.y;
      x2 = m_ImageScreenPos.x + m_ImageScreenSize.x;
      y2 = m_ImageScreenPos.y + m_ImageScreenSize.y;
    }

    float relX1 = x1 - m_ImageScreenPos.x;
    float relY1 = y1 - m_ImageScreenPos.y;
    float relX2 = x2 - m_ImageScreenPos.x;
    float relY2 = y2 - m_ImageScreenPos.y;

    float scaleX = (float) currentImg.width / m_ImageScreenSize.x;
    float scaleY = (float) currentImg.height / m_ImageScreenSize.y;

    SDL_Rect cropRect;
    cropRect.x = (int) (relX1 * scaleX);
    cropRect.y = (int) (relY1 * scaleY);
    cropRect.w = (int) ((relX2 - relX1) * scaleX);
    cropRect.h = (int) ((relY2 - relY1) * scaleY);

    if (cropRect.x < 0)
      cropRect.x = 0;
    if (cropRect.y < 0)
      cropRect.y = 0;
    if (cropRect.x + cropRect.w > currentImg.width)
      cropRect.w = currentImg.width - cropRect.x;
    if (cropRect.y + cropRect.h > currentImg.height)
      cropRect.h = currentImg.height - cropRect.y;

    if (cropRect.w <= 0 || cropRect.h <= 0)
      return {};

    auto croppedSurface = SDL::MakeSurface(cropRect.w, cropRect.h, currentImg.surface->format);
    if (!croppedSurface)
      return {};

    if (!SDL_BlitSurface(currentImg.surface, &cropRect, croppedSurface.get(), nullptr)) {
      return {};
    }

    std::vector<unsigned char> buffer;

    // We loaded the image as RGBA (4 channels)
    stbi_write_png_to_func(
        StbiWriteFunc, &buffer, croppedSurface->w, croppedSurface->h, 4, croppedSurface->pixels, croppedSurface->pitch);

    return buffer;
  }

  std::vector<unsigned char> ImageSection::GetFullImageBytes()
  {
    if (m_Images.empty() || m_CurrentImageIndex >= m_Images.size())
      return {};

    ImageData& currentImg = m_Images[m_CurrentImageIndex];
    if (!currentImg.surface)
      return {};

    std::vector<unsigned char> buffer;

    // We loaded the image as RGBA (4 channels)
    stbi_write_png_to_func(StbiWriteFunc,
                           &buffer,
                           currentImg.surface->w,
                           currentImg.surface->h,
                           4,
                           currentImg.surface->pixels,
                           currentImg.surface->pitch);

    return buffer;
  }

} // namespace Image2Card::UI
