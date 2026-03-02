cmake_minimum_required(VERSION 3.16)
project(FallingSand VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# === GLFW ===
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_INSTALL  OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# === GLAD ===
add_library(glad_gl_core_33 STATIC third_party/glad2/src/gl.c)
target_include_directories(glad_gl_core_33 PUBLIC third_party/glad2/include)

# === Ejecutable ===
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)

add_executable(FallingSand ${SRC})
set_target_properties(FallingSand PROPERTIES
    OUTPUT_NAME "Kobold Delvers"
)
target_include_directories(FallingSand PRIVATE src)

find_package(OpenGL REQUIRED)
target_link_libraries(FallingSand PRIVATE glfw glad_gl_core_33 OpenGL::GL)

# === Rutas SIEMPRE relativas al exe ===
target_compile_definitions(FallingSand PRIVATE
  SHADER_DIR="assets/shaders"
  AUDIO_DIR="assets/audios"
  SPRITE_DIR="assets/sprites"
)

# === Copia assets al lado del exe ===
add_custom_command(TARGET FallingSand POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:FallingSand>/assets"
)

add_custom_command(TARGET FallingSand POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/levels"
          "$<TARGET_FILE_DIR:FallingSand>/levels"
)

set_target_properties(FallingSand PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:FallingSand>"
)
