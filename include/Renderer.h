#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <map>
#include "Card.h"

class Renderer {
public:
    static Renderer& getInstance();
    bool init(const char* title, int width, int height);
    void shutdown();
    void clear();
    void present();
    void drawTable();
    void drawCard(const Card& card, int x, int y, int w, int h);
    void drawButton(const std::string& text, int x, int y, int w, int h, bool hover);
    void drawText(const std::string& text, int x, int y, SDL_Color color);
    bool isButtonClicked(const std::string& text, int x, int y, int w, int h);
    SDL_Renderer* getSDLRenderer();
    ~Renderer();

private:
    Renderer() = default;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    TTF_Font* m_font = nullptr;
    std::map<std::string, SDL_Texture*> m_textures;
    void loadCardTextures();
    static bool s_ttfInitialized;
};

#endif