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
find_package(GTest REQUIRED)

# add from https://stackoverflow.com/questions/76869634/linker-fails-when-using-libpqxx-with-cmake
find_package(PkgConfig REQUIRED)

pkg_check_modules(PQXX REQUIRED libpqxx)
# add from https://stackoverflow.com/questions/76869634/linker-fails-when-using-libpqxx-with-cmake

include_directories(include)
include_directories(test)

add_executable(dec-rep 
    src/transport_service.cpp
    src/file_watcher.cpp
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


target_link_libraries(dec-rep PRIVATE Boost::filesystem Boost::json ${PQXX_LINK_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(dec-rep PRIVATE efsw::efsw PostgreSQL::PostgreSQL)
target_link_libraries(dec-rep PRIVATE OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)
target_link_libraries(dec-rep PRIVATE GTest::gtest GTest::gtest_main)

add_executable(dec-rep-db_manager_test
    src/db_manager.cpp
    test/db_manager_test.cpp
)

target_link_libraries(dec-rep-db_manager_test PRIVATE Boost::filesystem Boost::json ${PQXX_LINK_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(dec-rep-db_manager_test PRIVATE PostgreSQL::PostgreSQL)
target_link_libraries(dec-rep-db_manager_test PRIVATE GTest::gtest GTest::gtest_main)