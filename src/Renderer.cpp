#include "Renderer.h"
#include <iostream>
#include <cstring>
#include <SDL3_image/SDL_image.h>

bool Renderer::s_ttfInitialized = false;

Renderer& Renderer::getInstance() {
    static Renderer instance;
    return instance;
}

bool Renderer::init(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_ClearError();

    if (!s_ttfInitialized) {
        if (!TTF_Init()) {
            std::cerr << "TTF_Init failed" << std::endl;
            const char* err = SDL_GetError();
            if (err && std::strlen(err) > 0) {
                std::cerr << "SDL_GetError: " << err << std::endl;
            } else {
                std::cerr << "SDL_GetError is empty. Possible missing dependencies." << std::endl;
                std::cerr << "Check that all required DLLs are in the executable directory." << std::endl;
            }
            return false;
        }
        s_ttfInitialized = true;
    }

    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return false;
    }


    const char* basePath = SDL_GetBasePath();
    std::string fontPath = (basePath ? std::string(basePath) : "") + "arial.ttf";
    m_font = TTF_OpenFont(fontPath.c_str(), 18);
    if (!m_font) {
        std::cerr << "Warning: font arial.ttf not found at " << fontPath << " - " << SDL_GetError() << std::endl;

        m_font = TTF_OpenFont("arial.ttf", 18);
        if (!m_font)
            std::cerr << "Also failed from current directory: " << SDL_GetError() << std::endl;
    }

    loadTextures();
    return true;
}

void Renderer::loadTextures() {
    const char* suits[] = {"clubs", "diamonds", "hearts", "spades"};
    const char* ranks[] = {"two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "jack", "queen", "king", "ace"};

    for (const char* rank : ranks) {
        for (const char* suit : suits) {
            std::string filename = std::string("texturi/") + rank + "_" + suit + ".png";
            std::string key = std::string(rank) + "_" + suit;
            SDL_Texture* tex = loadTexture(filename);
            if (tex) m_textures[key] = tex;
        }
    }
    SDL_Texture* backTex = loadTexture("texturi/back.png");
    if (backTex) m_textures["back"] = backTex;
}

SDL_Texture* Renderer::loadTexture(const std::string& path) {
    SDL_Surface* surf = IMG_Load(path.c_str());
    if (!surf) {
        std::cerr << "Failed to load texture: " << path << " - " << SDL_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surf);
    SDL_DestroySurface(surf);
    return tex;
}

void Renderer::shutdown() {
    for (auto& pair : m_textures) {
        SDL_DestroyTexture(pair.second);
    }
    m_textures.clear();
    if (m_font) TTF_CloseFont(m_font);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    m_font = nullptr;
    m_renderer = nullptr;
    m_window = nullptr;
}

void Renderer::drawTable() {
    int w, h;
    getWindowSize(w, h);
    SDL_SetRenderDrawColor(m_renderer, 15, 80, 25, 255);
    SDL_FRect table = {0, 0, (float)w, (float)h};
    SDL_RenderFillRect(m_renderer, &table);
    SDL_SetRenderDrawColor(m_renderer, 200, 180, 50, 255);
    SDL_FRect border = {40, 40, (float)w-80, (float)h-80};
    SDL_RenderRect(m_renderer, &border);
}

void Renderer::drawCard(const Card& card, int x, int y, int w, int h, bool hidden) {
    SDL_FRect rect = {(float)x, (float)y, (float)w, (float)h};
    if (hidden) {
        auto it = m_textures.find("back");
        if (it != m_textures.end()) {
            SDL_RenderTexture(m_renderer, it->second, nullptr, &rect);
        } else {
            SDL_SetRenderDrawColor(m_renderer, 20, 40, 150, 255);
            SDL_RenderFillRect(m_renderer, &rect);
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
            SDL_RenderRect(m_renderer, &rect);
            drawText("?", x + w/2 - 5, y + h/2 - 10, {255,255,255,255});
        }
    } else {
        std::string key = card.getTextureName();
        auto it = m_textures.find(key);
        if (it != m_textures.end()) {
            SDL_RenderTexture(m_renderer, it->second, nullptr, &rect);
        } else {
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(m_renderer, &rect);
            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
            SDL_RenderRect(m_renderer, &rect);
            SDL_Color textColor = card.isRed() ? SDL_Color{220,0,0,255} : SDL_Color{0,0,0,255};
            drawText(card.toString(), x + 5, y + 5, textColor);
        }
    }
}

void Renderer::drawButton(const std::string& text, int x, int y, int w, int h) {
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    bool hover = (mx >= x && mx <= x + w && my >= y && my <= y + h);
    SDL_Color bgColor = hover ? SDL_Color{200,50,50,255} : SDL_Color{120,30,30,255};
    SDL_SetRenderDrawColor(m_renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_FRect rect = {(float)x, (float)y, (float)w, (float)h};
    SDL_RenderFillRect(m_renderer, &rect);
    SDL_SetRenderDrawColor(m_renderer, 255, 215, 0, 255);
    SDL_RenderRect(m_renderer, &rect);
    drawText(text, x + w/2 - (int)(text.length() * 5), y + h/2 - 10, {255,255,255,255});
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    if (!m_font) return;
    SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), text.length(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FRect dest = {(float)x, (float)y, (float)surface->w, (float)surface->h};
    SDL_RenderTexture(m_renderer, texture, nullptr, &dest);
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

bool Renderer::isButtonClicked(int x, int y, int w, int h) {
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

void Renderer::clear() {
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::present() {
    SDL_RenderPresent(m_renderer);
}

SDL_Renderer* Renderer::getSDLRenderer() {
    return m_renderer;
}

void Renderer::getWindowSize(int& w, int& h) {
    if (m_window) {
        SDL_GetWindowSize(m_window, &w, &h);
    } else {
        w = 1024; h = 768;
    }
}

Renderer::~Renderer() {
    shutdown();
}
