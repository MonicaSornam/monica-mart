cmake_minimum_required(VERSION 3.16)
project(MonicaMart CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/filters
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
    src/main.cc
    src/models/DbManager.cc
    src/controllers/AuthController.cc
    src/controllers/ProductController.cc
    src/controllers/CartController.cc
    src/controllers/OrderController.cc
    src/controllers/ReviewController.cc
    src/controllers/SellerController.cc
    src/controllers/AdminController.cc
    src/controllers/ChatbotController.cc
)

# Define executable
add_executable(MonicaMart ${SOURCES})

# Link libraries
target_link_libraries(MonicaMart PRIVATE
    Drogon::Drogon
    SQLite::SQLite3
    Threads::Threads
)

# Custom copy target to copy web assets, database schema, and config to build directory
add_custom_command(TARGET MonicaMart POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/public
        $<TARGET_FILE_DIR:MonicaMart>/public
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/frontend
        $<TARGET_FILE_DIR:MonicaMart>/frontend
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/sql
        $<TARGET_FILE_DIR:MonicaMart>/sql
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/database
        $<TARGET_FILE_DIR:MonicaMart>/database
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/config/config.json
        $<TARGET_FILE_DIR:MonicaMart>/config.json
    COMMENT "Copying public web assets, sql schema, and config to binary folder"
)
