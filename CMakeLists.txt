cmake_minimum_required(VERSION 3.20)
project(myPokerProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Базовые пути к библиотекам
set(SDL3_BASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3-3.4.8/x86_64-w64-mingw32")
set(SDL3_TTF_BASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_ttf-3.2.2/x86_64-w64-mingw32")
set(SDL3_IMAGE_BASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_image-3.4.4/x86_64-w64-mingw32")
set(SDL3_NET_BASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_net-3.2.0/x86_64-w64-mingw32")

# Пути к заголовочным папкам
set(SDL3_INC_DIR "${SDL3_BASE}/include")
set(SDL3_TTF_INC_DIR "${SDL3_TTF_BASE}/include")
set(SDL3_IMAGE_INC_DIR "${SDL3_IMAGE_BASE}/include")
# Для SDL_net: заголовок лежит в include/SDL3_net/SDL_net.h
set(SDL3_NET_INC_DIR "${SDL3_NET_BASE}/include/SDL3_net")

# Собираем исходники
file(GLOB SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

# Создаём исполняемый файл
add_executable(MyPokerProject ${SOURCES} ${HEADERS}
        include/NetworkProtocol.h
        include/NetworkManager.h
        src/NetworkManager.cpp
        include/LANBlackjackGame.h
        src/LANBlackjackGame.cpp)

# Подключаем заголовочные файлы (после add_executable)
target_include_directories(MyPokerProject PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        ${SDL3_INC_DIR}
        ${SDL3_TTF_INC_DIR}
        ${SDL3_IMAGE_INC_DIR}
        ${SDL3_NET_INC_DIR}      # Теперь компилятор найдёт SDL_net.h
)

# Пути поиска .lib файлов
target_link_directories(MyPokerProject PRIVATE
        "${SDL3_BASE}/lib"
        "${SDL3_TTF_BASE}/lib"
        "${SDL3_IMAGE_BASE}/lib"
        "${SDL3_NET_BASE}/lib"
)

# Связывание с библиотеками
target_link_libraries(MyPokerProject PRIVATE
        SDL3
        SDL3_ttf
        SDL3_image
        SDL3_net
)

# Копирование ресурсов
add_custom_command(TARGET MyPokerProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/texturi"
        "$<TARGET_FILE_DIR:MyPokerProject>/texturi"
        COMMENT "Copying card textures to build directory..."
)

# Копирование шрифта (если есть)
add_custom_command(TARGET MyPokerProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/arial.ttf"
        "$<TARGET_FILE_DIR:MyPokerProject>/arial.ttf"
        COMMENT "Copying font file..."
)

# Копирование DLL для Windows
if(WIN32)
    add_custom_command(TARGET MyPokerProject POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_BASE}/bin/SDL3.dll"
            "${SDL3_TTF_BASE}/bin/SDL3_ttf.dll"
            "${SDL3_IMAGE_BASE}/bin/SDL3_image.dll"
            "${SDL3_NET_BASE}/bin/SDL3_net.dll"
            "$<TARGET_FILE_DIR:MyPokerProject>/"
            COMMENT "Copying SDL3 Runtime DLLs (including SDL3_net)..."
    )
endif()
