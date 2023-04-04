cmake_minimum_required(VERSION 3.1.0)

set(PROJECT_NAME nanovg)

project(${PROJECT_NAME})

find_package(OpenGL REQUIRED)
message(STATUS "OPENGL_FOUND:= ${OPENGL_FOUND}")
message(STATUS "OPENGL_INCLUDE_DIR:= ${OPENGL_INCLUDE_DIR}")


set(SRC_DIR "./src")
file(GLOB SRCS ${SRC_DIR}/*.c)
file(GLOB HDRS ${SRC_DIR}/*.h)

add_library(${PROJECT_NAME} ${SRCS})
if(APPLE)
  target_include_directories(nanovg PUBLIC
    ${OPENGL_INCLUDE_DIR}/Headers
  )
else()
  target_include_directories(nanovg PUBLIC ${OPENGL_INCLUDE_DIR}/GL)
endif()


install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include/${PROJECT_NAME}}
)

install(FILES ${HDRS} DESTINATION include/${PROJECT_NAME}/)
