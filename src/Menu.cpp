#include "Menu.h"
#include "Renderer.h"
#include <iostream>

bool Menu::isPointInRect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x <= rx+rw && y >= ry && y <= ry+rh;
}

void Menu::drawNumberField(const std::string& label, int value, int x, int y, bool active) {
    auto& r = Renderer::getInstance();
    SDL_Color bgColor = active ? SDL_Color{100,150,200,255} : SDL_Color{50,50,80,255};
    SDL_SetRenderDrawColor(r.getSDLRenderer(), bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_FRect rect = { (float)x, (float)y, 200, 50 };
    SDL_RenderFillRect(r.getSDLRenderer(), &rect);
    SDL_SetRenderDrawColor(r.getSDLRenderer(), 255, 255, 255, 255);
    SDL_RenderRect(r.getSDLRenderer(), &rect);
    r.drawText(label + ": " + std::to_string(value), x+10, y+10, {255,255,255,255});

    SDL_FRect minusRect = { (float)x+210, (float)y, 40, 50 };
    SDL_FRect plusRect  = { (float)x+260, (float)y, 40, 50 };
    SDL_RenderFillRect(r.getSDLRenderer(), &minusRect);
    SDL_RenderRect(r.getSDLRenderer(), &minusRect);
    SDL_RenderFillRect(r.getSDLRenderer(), &plusRect);
    SDL_RenderRect(r.getSDLRenderer(), &plusRect);
    r.drawText("-", x+220, y+10, {0,0,0,255});
    r.drawText("+", x+270, y+10, {0,0,0,255});
}

Menu::Resolution Menu::showResolutionMenu() {
    Resolution result{1024,768};
    const int btnW = 300, btnH = 60;
    const int startY = 200;
    const int stepY = 80;
    std::vector<std::pair<int,int>> options = {{1280,720},{1600,900},{1920,1080},{1024,768}};
    bool waiting = true;

    while (waiting) {
        Renderer::getInstance().clear();
        Renderer::getInstance().drawText("Select Window Resolution", 350, 100, {255,255,0,255});

        float mx, my;
        SDL_GetMouseState(&mx, &my);
        for (size_t i = 0; i < options.size(); ++i) {
            int x = (1024 - btnW)/2;
            int y = startY + i*stepY;
            std::string label = std::to_string(options[i].first) + "x" + std::to_string(options[i].second);
            bool hover = isPointInRect((int)mx, (int)my, x, y, btnW, btnH);
            Renderer::getInstance().drawButton(label, x, y, btnW, btnH, hover);
        }
        Renderer::getInstance().present();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { waiting = false; result = {1024,768}; break; }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int xm = (int)event.button.x, ym = (int)event.button.y;
                for (size_t i = 0; i < options.size(); ++i) {
                    int x = (1024 - btnW)/2;
                    int y = startY + i*stepY;
                    if (isPointInRect(xm, ym, x, y, btnW, btnH)) {
                        result.width = options[i].first;
                        result.height = options[i].second;
                        waiting = false;
                        break;
                    }
                }
            }
        }
        SDL_Delay(16);
    }
    return result;
}

Menu::Choice Menu::showMainMenu() {
    const int winW = 1024, winH = 768;
    const int btnW = 300, btnH = 80;
    const int btnX = (winW - btnW)/2;
    const int pokerY = 200, bjY = 320, quitY = 440;
    Choice result = Choice::None;

    while (result == Choice::None) {
        Renderer::getInstance().clear();
        Renderer::getInstance().drawText("Card Games Menu", 400, 80, {255,255,0,255});

        float mx, my;
        SDL_GetMouseState(&mx, &my);
        bool hoverPoker = isPointInRect((int)mx, (int)my, btnX, pokerY, btnW, btnH);
        bool hoverBJ    = isPointInRect((int)mx, (int)my, btnX, bjY, btnW, btnH);
        bool hoverQuit  = isPointInRect((int)mx, (int)my, btnX, quitY, btnW, btnH);

        Renderer::getInstance().drawButton("Texas Hold'em Poker", btnX, pokerY, btnW, btnH, hoverPoker);
        Renderer::getInstance().drawButton("Blackjack", btnX, bjY, btnW, btnH, hoverBJ);
        Renderer::getInstance().drawButton("Quit", btnX, quitY, btnW, btnH, hoverQuit);
        Renderer::getInstance().present();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) return Choice::Quit;
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int xm = (int)event.button.x, ym = (int)event.button.y;
                if (isPointInRect(xm, ym, btnX, pokerY, btnW, btnH)) return Choice::Poker;
                if (isPointInRect(xm, ym, btnX, bjY, btnW, btnH)) return Choice::Blackjack;
                if (isPointInRect(xm, ym, btnX, quitY, btnW, btnH)) return Choice::Quit;
            }
        }
        SDL_Delay(16);
    }
    return Choice::Quit;
}

Menu::PlayerSetup Menu::showPlayerSetupMenu() {
    PlayerSetup setup = {1,1};
    bool waiting = true;
    const int fieldX = 300;
    const int humansY = 250, botsY = 350;
    const int confirmX = (1024 - 200)/2, confirmY = 500, btnW = 200, btnH = 50;

    while (waiting) {
        Renderer::getInstance().clear();
        Renderer::getInstance().drawText("Player Setup", 400, 100, {255,255,0,255});
        drawNumberField("Humans", setup.humans, fieldX, humansY, false);
        drawNumberField("Bots",   setup.bots,   fieldX, botsY,   false);

        float mx, my;
        SDL_GetMouseState(&mx, &my);
        bool hoverStart = isPointInRect((int)mx, (int)my, confirmX, confirmY, btnW, btnH);
        Renderer::getInstance().drawButton("Start Game", confirmX, confirmY, btnW, btnH, hoverStart);
        Renderer::getInstance().present();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { waiting = false; setup = {0,0}; break; }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                int xm = (int)event.button.x, ym = (int)event.button.y;
                if (isPointInRect(xm, ym, fieldX+210, humansY, 40, 50)) setup.humans = std::min(10, setup.humans+1);
                else if (isPointInRect(xm, ym, fieldX+260, humansY, 40, 50)) setup.humans = std::max(0, setup.humans-1);
                else if (isPointInRect(xm, ym, fieldX+210, botsY, 40, 50)) setup.bots = std::min(10, setup.bots+1);
                else if (isPointInRect(xm, ym, fieldX+260, botsY, 40, 50)) setup.bots = std::max(0, setup.bots-1);
                else if (isPointInRect(xm, ym, confirmX, confirmY, btnW, btnH)) {
                    if (setup.humans + setup.bots > 0) waiting = false;
                }
            }
        }
        SDL_Delay(16);
    }
    return setup;
}