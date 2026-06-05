#ifndef MENU_H
#define MENU_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>

class Menu {
public:
    enum class GameChoice { Blackjack, Poker, Quit, None };

    struct Resolution { int width, height; };
    static Resolution showResolutionMenu();
    static GameChoice showMainMenu();

    struct PlayerSetup { int humans; int bots; };
    static PlayerSetup showPlayerSetupMenu();

private:
    static bool isPointInRect(int x, int y, int rx, int ry, int rw, int rh);
    static void drawNumberField(const std::string& label, int value, int x, int y);
};

#endif