project(audioplayground)
cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 14)

add_executable(
    ${PROJECT_NAME}
    "main.cpp"
    "audiosource.cpp"
)
