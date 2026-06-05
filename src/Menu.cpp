#include "Menu.h"
#include "Renderer.h"
#include <cstring>

bool Menu::isPointInRect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

void Menu::drawNumberField(const std::string& label, int value, int x, int y) {
    auto& r = Renderer::getInstance();
    SDL_SetRenderDrawColor(r.getSDLRenderer(), 50, 50, 80, 255);
    SDL_FRect rect = { (float)x, (float)y, 200, 50 };
    SDL_RenderFillRect(r.getSDLRenderer(), &rect);
    SDL_SetRenderDrawColor(r.getSDLRenderer(), 255, 255, 255, 255);
    SDL_RenderRect(r.getSDLRenderer(), &rect);
    r.drawText(label + ": " + std::to_string(value), x + 10, y + 10, { 255, 255, 255, 255 });

    SDL_FRect minusRect = { (float)x + 210, (float)y, 40, 50 };
    SDL_FRect plusRect = { (float)x + 260, (float)y, 40, 50 };
    SDL_SetRenderDrawColor(r.getSDLRenderer(), 200, 200, 200, 255);
    SDL_RenderFillRect(r.getSDLRenderer(), &minusRect);
    SDL_RenderFillRect(r.getSDLRenderer(), &plusRect);
    r.drawText("-", x + 225, y + 10, { 0, 0, 0, 255 });
    r.drawText("+", x + 275, y + 10, { 0, 0, 0, 255 });
}

Menu::Resolution Menu::showResolutionMenu() {
    bool waiting = true;
    Resolution res = { 1280, 720 };
    auto& r = Renderer::getInstance();

    while (waiting) {
        r.clear();
        r.drawText("Select Resolution", 400, 150, { 255, 255, 255, 255 });
        r.drawButton("1280x720", 400, 250, 220, 50);
        r.drawButton("1600x900", 400, 330, 220, 50);
        r.drawButton("1920x1080", 400, 410, 220, 50);
        r.present();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { waiting = false; break; }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int xm = (int)event.button.x, ym = (int)event.button.y;
                if (isPointInRect(xm, ym, 400, 250, 220, 50)) { res = { 1280, 720 }; waiting = false; }
                if (isPointInRect(xm, ym, 400, 330, 220, 50)) { res = { 1600, 900 }; waiting = false; }
                if (isPointInRect(xm, ym, 400, 410, 220, 50)) { res = { 1920, 1080 }; waiting = false; }
            }
        }
        SDL_Delay(16);
    }
    return res;
}

Menu::GameChoice Menu::showMainMenu() {
    bool waiting = true;
    auto& r = Renderer::getInstance();
    int w, h;
    r.getWindowSize(w, h);

    while (waiting) {
        r.clear();
        r.drawText("CASINO SUITE", w / 2 - 100, h / 2 - 200, { 255, 215, 0, 255 });
        r.drawButton("Blackjack", w / 2 - 100, h / 2 - 80, 200, 50);
        r.drawButton("Texas Hold'em Poker", w / 2 - 100, h / 2, 200, 50);
        r.drawButton("LAN Blackjack (Host)", w / 2 - 100, h / 2 + 80, 200, 50);
        r.drawButton("LAN Blackjack (Join)", w / 2 - 100, h / 2 + 140, 200, 50);
        r.drawButton("Quit", w / 2 - 100, h / 2 + 200, 200, 50);
        r.present();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) return GameChoice::Quit;
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int xm = (int)event.button.x, ym = (int)event.button.y;
                if (isPointInRect(xm, ym, w / 2 - 100, h / 2 - 80, 200, 50)) return GameChoice::Blackjack;
                if (isPointInRect(xm, ym, w / 2 - 100, h / 2, 200, 50)) return GameChoice::Poker;
                if (isPointInRect(xm, ym, w / 2 - 100, h / 2 + 80, 200, 50)) return GameChoice::LANBlackjackHost;
                if (isPointInRect(xm, ym, w / 2 - 100, h / 2 + 140, 200, 50)) return GameChoice::LANBlackjackJoin;
                if (isPointInRect(xm, ym, w / 2 - 100, h / 2 + 200, 200, 50)) return GameChoice::Quit;
            }
        }
        SDL_Delay(16);
    }
    return GameChoice::None;
}

std::string Menu::inputText(const std::string& title, const std::string& defaultValue, int x, int y) {
    auto& r = Renderer::getInstance();
    SDL_Window* window = r.getSDLWindow();
    SDL_StartTextInput(window);
    std::string input = defaultValue;
    bool waiting = true;

    while (waiting) {
        r.clear();
        r.drawText(title, x, y, {255,255,255,255});
        r.drawText(input + "_", x, y + 30, {200,200,200,255});
        r.present();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                waiting = false;
                break;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_RETURN) {
                    waiting = false;
                } else if (event.key.key == SDLK_BACKSPACE && !input.empty()) {
                    input.pop_back();
                }
            }
            if (event.type == SDL_EVENT_TEXT_INPUT) {
                input += event.text.text;
            }
        }
        SDL_Delay(16);
    }
    SDL_StopTextInput(window);
    return input;
}

Menu::PlayerSetup Menu::showPlayerSetupMenu() {
    bool waiting = true;
    PlayerSetup setup = { 1, 3 };
    auto& r = Renderer::getInstance();
    int winW, winH;
    r.getWindowSize(winW, winH);
    const int fieldX = winW / 2 - 150;
    const int humansY = winH / 2 - 80, botsY = winH / 2 + 20;
    const int confirmX = (winW - 200) / 2, confirmY = winH / 2 + 120, btnW = 200, btnH = 50;

    while (waiting) {
        r.clear();
        r.drawText("Player Setup", winW / 2 - 80, winH / 2 - 200, { 255, 255, 0, 255 });
        drawNumberField("Humans", setup.humans, fieldX, humansY);
        drawNumberField("Bots", setup.bots, fieldX, botsY);
        r.drawButton("Start Game", confirmX, confirmY, btnW, btnH);
        r.present();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { waiting = false; setup = { 0,0 }; break; }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int xm = (int)event.button.x, ym = (int)event.button.y;
                if (isPointInRect(xm, ym, fieldX + 210, humansY, 40, 50)) { if (setup.humans > 0) setup.humans--; }
                if (isPointInRect(xm, ym, fieldX + 260, humansY, 40, 50)) { if (setup.humans + setup.bots < 7) setup.humans++; }
                if (isPointInRect(xm, ym, fieldX + 210, botsY, 40, 50)) { if (setup.bots > 0) setup.bots--; }
                if (isPointInRect(xm, ym, fieldX + 260, botsY, 40, 50)) { if (setup.humans + setup.bots < 7) setup.bots++; }
                if (isPointInRect(xm, ym, confirmX, confirmY, btnW, btnH)) {
                    if (setup.humans + setup.bots > 0) waiting = false;
                }
            }
        }
        SDL_Delay(16);
    }
    return setup;
}