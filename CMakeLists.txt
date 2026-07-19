cmake_minimum_required(VERSION 3.16)

project(GraphAnalytics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

add_executable(graph_analytics
    src/Base/MetaData.cpp
    src/IO/CsvReader.cpp
    src/IO/GraphConvertor.cpp
    src/IO/MyReader.cpp
    src/IO/MyWriter.cpp
    src/Execute/Executor.cpp
    src/Execute/main.cpp
)

target_include_directories(graph_analytics PRIVATE
    src
)

target_link_libraries(graph_analytics PRIVATE
    OpenMP::OpenMP_CXX
)
