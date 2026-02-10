#!/bin/bash
# setup_project.sh

PROJECT_ROOT="."

# Создаём структуру папок
mkdir -p "$PROJECT_ROOT"/{include/mine/{core,graphics,scene,physics,utils},src/{core,graphics,scene,physics},assets/{textures,shaders,maps},third_party/{glfw,glad,glm,stb}}

echo "✅ Структура проекта создана"

# Скачиваем stb_image.h
curl -o "$PROJECT_ROOT/third_party/stb/stb_image.h" https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

if [ $? -eq 0 ]; then
    echo "✅ stb_image.h скачан"
else
    echo "❌ Ошибка при скачивании stb_image.h"
    exit 1
fi

# Инициализируем git и подмодули (опционально)
if [ ! -d "$PROJECT_ROOT/.git" ]; then
    git init
    git submodule add https://github.com/glfw/glfw "$PROJECT_ROOT/third_party/glfw" 2>/dev/null || echo "glfw submodule exists"
    git submodule add https://github.com/g-truc/glm "$PROJECT_ROOT/third_party/glm" 2>/dev/null || echo "glm submodule exists"
    git submodule update --init --recursive
fi

echo -e "\n📁 Структура проекта:"
tree "$PROJECT_ROOT" -L 3 -I ".git|build" 2>/dev/null || find "$PROJECT_ROOT" -type d -maxdepth 3 | head -30

echo -e "\n🚀 Готово! Теперь можно:"
echo "   cd $PROJECT_ROOT"
echo "   mkdir build && cd build"
echo "   cmake .."
