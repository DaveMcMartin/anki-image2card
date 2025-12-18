#include "ui/ImageSection.h"
#include "language/ILanguage.h"
#include "config/ConfigManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include "core/Logger.h"
#include "core/sdl/SDLWrappers.h"
#include "IconsFontAwesome6.h"

namespace Image2Card::UI
{

ImageSection::ImageSection(SDL_Renderer* renderer,
                           std::vector<std::unique_ptr<Language::ILanguage>>* languages,
                           Language::ILanguage** activeLanguage,
                           Config::ConfigManager* configManager)
    : m_Renderer(renderer)
    , m_ImageTexture(nullptr)
    , m_ImageSurface(nullptr)
    , m_ImageWidth(0)
    , m_ImageHeight(0)
    , m_Languages(languages)
    , m_ActiveLanguage(activeLanguage)
    , m_ConfigManager(configManager)
{
}

ImageSection::~ImageSection()
{
    ClearImage();
}

void ImageSection::ClearImage()
{
    if (m_ImageTexture)
    {
        SDL_DestroyTexture(m_ImageTexture);
        m_ImageTexture = nullptr;
    }
    if (m_ImageSurface)
    {
        SDL_DestroySurface(m_ImageSurface);
        m_ImageSurface = nullptr;
    }
    m_ImageWidth = 0;
    m_ImageHeight = 0;
    m_SelectionStart = {0.0f, 0.0f};
    m_SelectionEnd = {0.0f, 0.0f};
}

void ImageSection::LoadImageFromFile(const std::string& path)
{
    ClearImage();

    int width{}, height{}, channels{};
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data)
    {
        AF_ERROR("Failed to load image: {}", path);
        return;
    }

    auto tempSurface = SDL::MakeSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, width * 4);
    if (!tempSurface)
    {
        AF_ERROR("Failed to create surface: {}", SDL_GetError());
        stbi_image_free(data);
        return;
    }

    m_ImageSurface = SDL_DuplicateSurface(tempSurface.get());
    stbi_image_free(data);

    if (!m_ImageSurface)
    {
        AF_ERROR("Failed to duplicate surface: {}", SDL_GetError());
        return;
    }

    m_ImageTexture = SDL_CreateTextureFromSurface(m_Renderer, m_ImageSurface);
    if (!m_ImageTexture)
    {
        AF_ERROR("Failed to create texture: {}", SDL_GetError());
    }
    else
    {
        m_ImageWidth = width;
        m_ImageHeight = height;
    }
}

void ImageSection::Update()
{
}

void ImageSection::Render()
{
    ImGui::Begin("Image Section", nullptr, ImGuiWindowFlags_NoScrollbar);

    if (m_Languages && m_ActiveLanguage && !m_Languages->empty())
    {
        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("Language", (*m_ActiveLanguage)->GetName().c_str()))
        {
            for (auto& language : *m_Languages)
            {
                bool isSelected = (language.get() == *m_ActiveLanguage);
                if (ImGui::Selectable(language->GetName().c_str(), isSelected))
                {
                    *m_ActiveLanguage = language.get();
                    if (m_ConfigManager)
                    {
                        auto& config = m_ConfigManager->GetConfig();
                        config.SelectedLanguage = language->GetIdentifier();
                        m_ConfigManager->Save();
                    }
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    float footerHeight = 40.0f;
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    availSize.y -= footerHeight;

    if (availSize.y < 10.0f) availSize.y = 10.0f;

    float startY = ImGui::GetCursorPosY();

    if (m_ImageTexture)
    {
        float aspect = (float)m_ImageWidth / (float)m_ImageHeight;
        float viewAspect = availSize.x / availSize.y;

        ImVec2 imageSize = availSize;
        if (aspect > viewAspect)
        {
            imageSize.y = availSize.x / aspect;
        }
        else
        {
            imageSize.x = availSize.y * aspect;
        }

        float cursorX = ImGui::GetCursorPosX() + (availSize.x - imageSize.x) * 0.5f;
        float cursorY = ImGui::GetCursorPosY() + (availSize.y - imageSize.y) * 0.5f;

        ImGui::SetCursorPosX(cursorX);
        ImGui::SetCursorPosY(cursorY);

        ImGui::Image((ImTextureID)m_ImageTexture, imageSize);

        m_ImageScreenPos = ImVec2(cursorX, cursorY);
        m_ImageScreenSize = imageSize;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;
        bool isHovered = ImGui::IsItemHovered();

        if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_IsSelecting = true;
            m_SelectionStart = mousePos;
            m_SelectionEnd = mousePos;
        }

        if (m_IsSelecting && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            m_SelectionEnd = mousePos;

            if (m_SelectionEnd.x < m_ImageScreenPos.x) m_SelectionEnd.x = m_ImageScreenPos.x;
            if (m_SelectionEnd.y < m_ImageScreenPos.y) m_SelectionEnd.y = m_ImageScreenPos.y;
            if (m_SelectionEnd.x > m_ImageScreenPos.x + m_ImageScreenSize.x) m_SelectionEnd.x = m_ImageScreenPos.x + m_ImageScreenSize.x;
            if (m_SelectionEnd.y > m_ImageScreenPos.y + m_ImageScreenSize.y) m_SelectionEnd.y = m_ImageScreenPos.y + m_ImageScreenSize.y;
        }
        else if (m_IsSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_IsSelecting = false;
        }

        if (m_SelectionStart.x != m_SelectionEnd.x || m_SelectionStart.y != m_SelectionEnd.y)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRect(m_SelectionStart, m_SelectionEnd, IM_COL32(0, 255, 0, 255), 0.0f, 0, 2.0f);
            drawList->AddRectFilled(m_SelectionStart, m_SelectionEnd, IM_COL32(0, 255, 0, 50));
        }
    }
    else
    {
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

    if (ImGui::Button(ICON_FA_TRASH " Clear", ImVec2(100, 0)))
    {
        ClearImage();
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.59f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.69f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.49f, 0.10f, 1.0f));
    if (ImGui::Button(ICON_FA_WAND_MAGIC_SPARKLES " Scan", ImVec2(100, 0)))
    {
        if (m_OnScanCallback)
        {
            m_OnScanCallback();
        }
        else
        {
            AF_WARN("Scan requested (no callback)");
        }
    }
    ImGui::PopStyleColor(3);

    ImGui::End();
}

static void StbiWriteFunc(void *context, void *data, int size)
{
    auto* vec = static_cast<std::vector<unsigned char>*>(context);
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    vec->insert(vec->end(), bytes, bytes + size);
}

std::vector<unsigned char> ImageSection::GetSelectedImageBytes()
{
    if (!m_ImageSurface) return {};

    float x1 = std::min(m_SelectionStart.x, m_SelectionEnd.x);
    float y1 = std::min(m_SelectionStart.y, m_SelectionEnd.y);
    float x2 = std::max(m_SelectionStart.x, m_SelectionEnd.x);
    float y2 = std::max(m_SelectionStart.y, m_SelectionEnd.y);

    if ((x2 - x1) < 1.0f || (y2 - y1) < 1.0f)
    {
        x1 = m_ImageScreenPos.x;
        y1 = m_ImageScreenPos.y;
        x2 = m_ImageScreenPos.x + m_ImageScreenSize.x;
        y2 = m_ImageScreenPos.y + m_ImageScreenSize.y;
    }

    float relX1 = x1 - m_ImageScreenPos.x;
    float relY1 = y1 - m_ImageScreenPos.y;
    float relX2 = x2 - m_ImageScreenPos.x;
    float relY2 = y2 - m_ImageScreenPos.y;

    float scaleX = (float)m_ImageWidth / m_ImageScreenSize.x;
    float scaleY = (float)m_ImageHeight / m_ImageScreenSize.y;

    SDL_Rect cropRect;
    cropRect.x = (int)(relX1 * scaleX);
    cropRect.y = (int)(relY1 * scaleY);
    cropRect.w = (int)((relX2 - relX1) * scaleX);
    cropRect.h = (int)((relY2 - relY1) * scaleY);

    if (cropRect.x < 0) cropRect.x = 0;
    if (cropRect.y < 0) cropRect.y = 0;
    if (cropRect.x + cropRect.w > m_ImageWidth) cropRect.w = m_ImageWidth - cropRect.x;
    if (cropRect.y + cropRect.h > m_ImageHeight) cropRect.h = m_ImageHeight - cropRect.y;

    if (cropRect.w <= 0 || cropRect.h <= 0) return {};

    auto croppedSurface = SDL::MakeSurface(cropRect.w, cropRect.h, m_ImageSurface->format);
    if (!croppedSurface) return {};

    if (!SDL_BlitSurface(m_ImageSurface, &cropRect, croppedSurface.get(), nullptr))
    {
        return {};
    }

    std::vector<unsigned char> buffer;

    // We loaded the image as RGBA (4 channels)
    stbi_write_png_to_func(StbiWriteFunc, &buffer, croppedSurface->w, croppedSurface->h, 4, croppedSurface->pixels, croppedSurface->pitch);

    return buffer;
}

std::vector<unsigned char> ImageSection::GetFullImageBytes()
{
    if (!m_ImageSurface) return {};

    std::vector<unsigned char> buffer;

    // We loaded the image as RGBA (4 channels)
    stbi_write_png_to_func(StbiWriteFunc, &buffer, m_ImageSurface->w, m_ImageSurface->h, 4, m_ImageSurface->pixels, m_ImageSurface->pitch);

    return buffer;
}

} // namespace Image2Card::UI
