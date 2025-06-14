cmake_minimum_required(VERSION 3.22)
project(dec-rep CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(default-options.cmake)

set(CMAKE_CXX_VISIBILITY_PRESET hidden) 


find_package(Boost 1.87 REQUIRED filesystem json)

include_directories(include)

add_executable(dec-rep
    FileWatcher/FileWatcher.cpp
    src/search_service.cpp
    src/change_propagator.cpp
    src/main.cpp
    src/db_manager.cpp
    src/process_events.cpp
    src/server.cpp
    src/client.cpp
    src/file_watcher.cpp
    src/dec_rep.cpp
    src/dec_rep_fs.cpp
)

target_link_libraries(dec-rep PRIVATE Boost::filesystem Boost::json ${CMAKE_DL_LIBS})