cmake_minimum_required(VERSION 3.10)


project(cbittorrent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD c99)
add_definitions("-Wall -g")
set(CMAKE_BUILD_TOOL Debug)

find_package(OpenSSL REQUIRED )


add_executable(
  ${PROJECT_NAME}
  main.c
  torrentfile.c
  bitfield.c
)

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL OpenSSL::Crypto)
