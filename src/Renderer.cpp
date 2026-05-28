#include "Renderer.h"
#include <iostream>

bool Renderer::s_ttfInitialized = false;

Renderer& Renderer::getInstance() {
    static Renderer instance;
    return instance;
}

bool Renderer::init(const char* title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    if (!s_ttfInitialized) {
        if (TTF_Init() < 0) {
            std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }
        s_ttfInitialized = true;
    }

    m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }

    m_font = TTF_OpenFont("arial.ttf", 24);
    if (!m_font) std::cerr << "Warning: font not loaded: " << SDL_GetError() << std::endl;

    loadCardTextures();
    return true;
}

void Renderer::shutdown() {
    if (m_font) TTF_CloseFont(m_font);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    m_font = nullptr;
    m_renderer = nullptr;
    m_window = nullptr;
}

void Renderer::loadCardTextures() {}

void Renderer::drawTable() {
    SDL_SetRenderDrawColor(m_renderer, 34, 139, 34, 255);
    SDL_FRect table = { 50, 50, 924, 668 };
    SDL_RenderFillRect(m_renderer, &table);
    SDL_SetRenderDrawColor(m_renderer, 139, 69, 19, 255);
    SDL_RenderRect(m_renderer, &table);
}

void Renderer::drawCard(const Card& card, int x, int y, int w, int h) {
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderFillRect(m_renderer, &rect);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderRect(m_renderer, &rect);
    drawText(card.toString(), x + w/4, y + h/4, {0,0,0,255});
}

void Renderer::drawButton(const std::string& text, int x, int y, int w, int h, bool hover) {
    SDL_Color bgColor = hover ? SDL_Color{100,150,200,255} : SDL_Color{50,50,200,255};
    SDL_SetRenderDrawColor(m_renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderFillRect(m_renderer, &rect);
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderRect(m_renderer, &rect);
    drawText(text, x + w/2 - (int)(text.length() * 6), y + h/2 - 10, {255,255,255,255});
}

void Renderer::drawText(const std::string& text, int x, int y, SDL_Color color) {
    if (!m_font) return;
    SDL_Surface* surface = TTF_RenderText_Blended(m_font, text.c_str(), text.length(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FRect dest = { (float)x, (float)y, (float)surface->w, (float)surface->h };
    SDL_RenderTexture(m_renderer, texture, nullptr, &dest);
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
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

bool Renderer::isButtonClicked(const std::string& /*text*/, int x, int y, int w, int h) {
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

Renderer::~Renderer() {
    if (m_font) TTF_CloseFont(m_font);
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
    if (s_ttfInitialized) {
        TTF_Quit();
        s_ttfInitialized = false;
    }
    SDL_Quit();
}