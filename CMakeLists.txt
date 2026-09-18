cmake_minimum_required(VERSION 3.16)
project(MonicaMart CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/backend
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/models
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/filters
)

# Find Drogon and dependencies
find_package(Drogon REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# Collect all backend source files
set(SOURCES
    backend/main.cc
    backend/models/DbManager.cc
    backend/controllers/AuthController.cc
    backend/controllers/ProductController.cc
    backend/controllers/CartController.cc
    backend/controllers/OrderController.cc
    backend/controllers/ReviewController.cc
    backend/controllers/SellerController.cc
    backend/controllers/AdminController.cc
    backend/controllers/ChatbotController.cc
)

# Define executable
add_executable(MonicaMart ${SOURCES})

# Link libraries
target_link_libraries(MonicaMart PRIVATE
    Drogon::Drogon
    SQLite::SQLite3
    Threads::Threads
)

# Custom copy target to copy frontend and database resources to build directory
add_custom_command(TARGET MonicaMart POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/frontend
        $<TARGET_FILE_DIR:MonicaMart>/frontend
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/database
        $<TARGET_FILE_DIR:MonicaMart>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/backend/config.json
        $<TARGET_FILE_DIR:MonicaMart>/config.json
    COMMENT "Copying frontend, database schema, and config to binary folder"
)
