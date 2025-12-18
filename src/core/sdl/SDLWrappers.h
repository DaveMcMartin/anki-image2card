#pragma once

#include <SDL3/SDL.h>
#include <memory>

namespace Image2Card::SDL
{

struct WindowDeleter
{
    void operator()(SDL_Window* window) const
    {
        if (window)
        {
            SDL_DestroyWindow(window);
        }
    }
};

struct RendererDeleter
{
    void operator()(SDL_Renderer* renderer) const
    {
        if (renderer)
        {
            SDL_DestroyRenderer(renderer);
        }
    }
};

struct SurfaceDeleter
{
    void operator()(SDL_Surface* surface) const
    {
        if (surface)
        {
            SDL_DestroySurface(surface);
        }
    }
};

struct TextureDeleter
{
    void operator()(SDL_Texture* texture) const
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
        }
    }
};

struct NonOwningDeleter
{
    void operator()(SDL_Surface* surface) const
    {
        if (surface)
        {
            SDL_free(surface);
        }
    }
};

using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;
using SurfacePtr = std::unique_ptr<SDL_Surface, SurfaceDeleter>;
using NonOwningSurfacePtr = std::unique_ptr<SDL_Surface, NonOwningDeleter>;
using TexturePtr = std::unique_ptr<SDL_Texture, TextureDeleter>;

inline WindowPtr MakeWindow(const char* title, int width, int height, SDL_WindowFlags flags)
{
    return WindowPtr(SDL_CreateWindow(title, width, height, flags));
}

inline RendererPtr MakeRenderer(SDL_Window* window, const char* name = nullptr)
{
    return RendererPtr(SDL_CreateRenderer(window, name));
}

inline SurfacePtr MakeSurface(int width, int height, SDL_PixelFormat format)
{
    return SurfacePtr(SDL_CreateSurface(width, height, format));
}

inline NonOwningSurfacePtr MakeSurfaceFrom(int width, int height, SDL_PixelFormat format, void* pixels, int pitch)
{
    return NonOwningSurfacePtr(SDL_CreateSurfaceFrom(width, height, format, pixels, pitch));
}

inline SurfacePtr DuplicateSurface(SDL_Surface* surface)
{
    if (!surface) return nullptr;
    return SurfacePtr(SDL_DuplicateSurface(surface));
}

inline TexturePtr MakeTexture(SDL_Renderer* renderer, SDL_Surface* surface)
{
    return TexturePtr(SDL_CreateTextureFromSurface(renderer, surface));
}

} // namespace Image2Card::SDL
