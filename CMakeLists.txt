cmake_minimum_required(VERSION 3.15)
project(asio_course CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(asio REQUIRED CONFIG)
find_package(fmt REQUIRED CONFIG)
find_package(nlohmann_json REQUIRED CONFIG)

add_executable(${PROJECT_NAME} src/main.cpp)

# target_compile_definitions(${PROJECT_NAME} PUBLIC -DASIO_ENABLE_HANDLER_TRACKING)

target_link_libraries(${PROJECT_NAME} PRIVATE asio::asio)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt-header-only)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

install(TARGETS ${PROJECT_NAME} DESTINATION "."
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )



