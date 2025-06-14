cmake_minimum_required(VERSION 3.22)
project(dec-rep CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# include(default-options.cmake)

set(CMAKE_CXX_VISIBILITY_PRESET hidden) 


find_package(Boost 1.87 REQUIRED filesystem json)
list(APPEND CMAKE_PREFIX_PATH "/home/likurinist/C++Progect/efsw")
find_package(efsw REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)


# add from https://stackoverflow.com/questions/76869634/linker-fails-when-using-libpqxx-with-cmake
find_package(PkgConfig REQUIRED)

pkg_check_modules(PQXX REQUIRED libpqxx)
# add from https://stackoverflow.com/questions/76869634/linker-fails-when-using-libpqxx-with-cmake

include_directories(include)

add_executable(dec-rep
    src/transport_service.cpp
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

target_link_libraries(dec-rep PRIVATE Boost::filesystem Boost::json efsw::efsw)
target_link_libraries(dec-rep PRIVATE ${PQXX_LINK_LIBRARIES} PostgreSQL::PostgreSQL ${CMAKE_DL_LIBS})
target_link_libraries(dec-rep PRIVATE OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)