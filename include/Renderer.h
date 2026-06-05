#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <unordered_map>
#include "Card.h"

class Renderer {
public:
    static Renderer& getInstance();

    bool init(const char* title, int width, int height);
    void shutdown();
    void clear();
    void present();

    void drawTable();
    void drawCard(const Card& card, int x, int y, int w, int h, bool hidden);
    void drawButton(const std::string& text, int x, int y, int w, int h);
    void drawText(const std::string& text, int x, int y, SDL_Color color);
    bool isButtonClicked(int x, int y, int w, int h);

    SDL_Renderer* getSDLRenderer();
    void getWindowSize(int& w, int& h);

    ~Renderer();

private:
    Renderer() = default;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    TTF_Font* m_font = nullptr;
    std::unordered_map<std::string, SDL_Texture*> m_textures;
    static bool s_ttfInitialized;

    void loadTextures();
    SDL_Texture* loadTexture(const std::string& path);
};

#endif