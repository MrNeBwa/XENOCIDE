cmake_minimum_required(VERSION 3.16)
project(XENOCIDE VERSION 1.0 LANGUAGES CXX C)  # ← КЛЮЧЕВОЕ ИЗМЕНЕНИЕ: добавлен язык C

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GLFW как подмодуль
add_subdirectory(third_party/glfw)

# Пути к заголовкам
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    third_party/glm
    third_party
)

# Исходники проекта
file(GLOB_RECURSE SOURCES 
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Исполняемый файл
add_executable(xenocide ${SOURCES})

target_link_libraries(xenocide PRIVATE
    glfw
    ${CMAKE_DL_LIBS}
)

# Линковка OpenGL
if(APPLE)
    target_link_libraries(xenocide PRIVATE "-framework OpenGL")
elseif(UNIX)
    target_link_libraries(xenocide PRIVATE GL)
elseif(WIN32)
    target_link_libraries(xenocide PRIVATE opengl32)
endif()

# Копируем ассеты в build/
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
