cmake_minimum_required(VERSION 3.22)

project(dec-rep CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(default-options.cmake)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)  # Recommended by Boost.DLL's manual

find_package(Boost 1.87 REQUIRED filesystem)  # Required by Boost.DLL
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

find_package(nlohmann_json 3.10.5 REQUIRED)

include_directories(include)

add_executable(dec-rep src/main.cpp src/db_manager.cpp src/process_events.cpp src/server.cpp src/client.cpp src/file_watcher.cpp src/dec_rep.cpp src/dec_rep_fs.cpp
        include/search_service.hpp
        src/search_service.cpp
        include/transport_service.hpp
        src/transport_service.cpp)
target_link_libraries(dec-rep ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(dec-rep OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(dec-rep ZLIB::ZLIB)