cmake_minimum_required(VERSION 3.20)
project(myPokerProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Базовые пути к библиотекам (ориентируемся на структуру вашей папки lib)
set(SDL3_BASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3-3.4.8/x86_64-w64-mingw32")
set(SDL3_TTF_BASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_ttf-3.2.2/x86_64-w64-mingw32")
set(SDL3_IMAGE_BASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_image-3.4.4/x86_64-w64-mingw32")

# Пути к заголовкам
set(SDL3_INC_DIR "${SDL3_BASE}/include")
set(SDL3_TTF_INC_DIR "${SDL3_TTF_BASE}/include")
set(SDL3_IMAGE_INC_DIR "${SDL3_IMAGE_BASE}/include")

file(GLOB SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)
file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_executable(MyPokerProject ${SOURCES} ${HEADERS})

target_include_directories(MyPokerProject PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        ${SDL3_INC_DIR}
        ${SDL3_TTF_INC_DIR}
        ${SDL3_IMAGE_INC_DIR}
)

# Настройка путей поиска библиотек .lib
target_link_directories(MyPokerProject PRIVATE
        "${SDL3_BASE}/lib"
        "${SDL3_TTF_BASE}/lib"
        "${SDL3_IMAGE_BASE}/lib"
)

target_link_libraries(MyPokerProject PRIVATE
        SDL3
        SDL3_ttf
        SDL3_image
)

# Автоматическое копирование ресурсов и DLL в папку сборки (Debug/Release)
add_custom_command(TARGET MyPokerProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/texturi"
        "$<TARGET_FILE_DIR:MyPokerProject>/texturi"
        COMMENT "Copying card textures to build directory..."
)

# Копирование шрифта arial.ttf, если он есть в папке проекта
add_custom_command(TARGET MyPokerProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/arial.ttf"
        "$<TARGET_FILE_DIR:MyPokerProject>/arial.ttf"
        COMMENT "Copying font file..."
)

if(WIN32)
    add_custom_command(TARGET MyPokerProject POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_BASE}/bin/SDL3.dll"
            "${SDL3_TTF_BASE}/bin/SDL3_ttf.dll"
            "${SDL3_IMAGE_BASE}/bin/SDL3_image.dll"
            "$<TARGET_FILE_DIR:MyPokerProject>/"
            COMMENT "Copying SDL3 Runtime DLLs..."
    )
endif()
