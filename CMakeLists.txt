cmake_minimum_required(VERSION 3.10)


project(cbittorrent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD c99)


add_executable(
  ${PROJECT_NAME}
  main.c
  torrentfile.c
)
