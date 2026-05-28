cmake_minimum_required(VERSION 3.20)
project(MyPokerProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========== ИСПРАВЬТЕ ЭТИ ПУТИ ПОД СВОЮ СИСТЕМУ ==========
set(SDL3_ROOT "C:/Users/Пользователь/CLionProjects/game/lib/SDL3-3.4.8/x86_64-w64-mingw32")
set(SDL3_TTF_ROOT "C:/Users/Пользователь/CLionProjects/myPokerProject/lib/SDL3_ttf-3.2.2/x86_64-w64-mingw32")
# ========================================================

include_directories(include)
include_directories(${SDL3_ROOT}/include)
include_directories(${SDL3_TTF_ROOT}/include)

link_directories(${SDL3_ROOT}/lib)
link_directories(${SDL3_TTF_ROOT}/lib)

add_executable(MyPokerProject
        main.cpp
        src/Card.cpp
        src/Deck.cpp
        src/Player.cpp
        src/Game.cpp
        src/PokerGame.cpp
        src/BlackjackGame.cpp
        src/Renderer.cpp
        src/Menu.cpp
)

target_link_libraries(MyPokerProject SDL3 SDL3_ttf)

add_custom_command(TARGET MyPokerProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL3_ROOT}/bin/SDL3.dll
        ${SDL3_TTF_ROOT}/bin/SDL3_ttf.dll
        $<TARGET_FILE_DIR:MyPokerProject>)