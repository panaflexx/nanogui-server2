cmake_minimum_required(VERSION 3.1.0)

set(PROJECT_NAME nanovg)

project(${PROJECT_NAME})

find_package(OpenGL REQUIRED)
message(STATUS "OPENGL_FOUND:= ${OPENGL_FOUND}")
message(STATUS "OPENGL_INCLUDE_DIR:= ${OPENGL_INCLUDE_DIR}")

set(GLFW_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../glfw" CACHE PATH "Path to GLFW source")

# Add GLFW subdirectory with a binary directory
add_subdirectory(${GLFW_SRC_DIR} ${CMAKE_CURRENT_BINARY_DIR}/glfw)

# Fontstash support
option(NANOVG_USE_FONTSTASH "Use fontstash with FreeType" ON)
set(NANOVG_LIBS "")

if (NANOVG_USE_FONTSTASH)
    add_definitions(-DFONS_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    list(APPEND NANOVG_LIBS ${FREETYPE_LIBRARIES})
    include_directories(${FREETYPE_INCLUDE_DIRS})

    find_package(PkgConfig)
    pkg_check_modules(HARFBUZZ harfbuzz)
    if (HARFBUZZ_FOUND)
        message(STATUS "HARFBUZZ_LIBRARIES (pkg-config): ${HARFBUZZ_LIBRARIES}")
        message(STATUS "HARFBUZZ_INCLUDE_DIRS (pkg-config): ${HARFBUZZ_INCLUDE_DIRS}")
        list(APPEND NANOVG_LIBS ${HARFBUZZ_LIBRARIES})
        include_directories(${HARFBUZZ_INCLUDE_DIRS})
    endif()
endif()

set(SRC_DIR "./src")
file(GLOB SRCS ${SRC_DIR}/*.c)
list(FILTER SRCS EXCLUDE REGEX "imagestash_test\\.c$") # Exclude imagestash_test.c from library
file(GLOB HDRS ${SRC_DIR}/*.h)

add_library(${PROJECT_NAME} ${SRCS})
if(APPLE)
    target_include_directories(nanovg PUBLIC
        ${OPENGL_INCLUDE_DIR}/Headers
    )
else()
    target_include_directories(nanovg PUBLIC ${OPENGL_INCLUDE_DIR}/GL)
endif()

# Ensure nanovg can find GLFW headers
target_include_directories(nanovg PUBLIC ${GLFW_SRC_DIR}/include)

# Link necessary macOS frameworks and fontstash libraries for nanovg
if(APPLE)
    target_link_libraries(nanovg PUBLIC
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
endif()
target_link_libraries(nanovg PUBLIC ${NANOVG_LIBS})

# Suppress implicit function declaration warnings for C99
if(APPLE)
    target_compile_options(nanovg PRIVATE -Wno-implicit-function-declaration)
endif()

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/${PROJECT_NAME}
)

install(FILES ${HDRS} DESTINATION include/${PROJECT_NAME}/)

# Build the test executable
add_executable(imagestash_test ${SRC_DIR}/imagestash_test.c)
target_include_directories(imagestash_test PRIVATE 
    ${SRC_DIR}
    ${GLFW_SRC_DIR}/include
)
target_link_libraries(imagestash_test PRIVATE nanovg glfw OpenGL::GL ${NANOVG_LIBS})

# Ensure GLFW uses OpenGL 3.2 for NanoVG GL3
target_compile_definitions(imagestash_test PRIVATE NANOVG_GL3=1)
if(APPLE)
    target_compile_options(imagestash_test PRIVATE -DGL_SILENCE_DEPRECATION -Wno-implicit-function-declaration)
    target_link_libraries(imagestash_test PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
    )
endif()
