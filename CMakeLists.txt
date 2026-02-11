cmake_minimum_required(VERSION 3.16)
project(XENOCIDE VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GLFW
add_subdirectory(third_party/glfw)

# Заголовки
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    third_party/glm
    third_party
)

# Исходники
set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/graphics/Renderer.cpp
    src/graphics/Shader.cpp
    src/graphics/Texture.cpp
)

# Исполняемый файл
add_executable(xenocide ${SOURCES})

target_link_libraries(xenocide PRIVATE
    glfw
    ${CMAKE_DL_LIBS}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(xenocide PRIVATE GL)
elseif(APPLE)
    target_link_libraries(xenocide PRIVATE "-framework OpenGL")
endif()

# Копируем ассеты
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
