#ifndef DEC_REP_FS_HPP_
#define DEC_REP_FS_HPP_

#include <set>
#include <string>
#include <unordered_map>

struct Node {
    std::string path{};
    
};

struct File : Node {};

struct Directory : Node {
    std::set<File> children;
};

struct DecRepFS {
    std::set<File> files;

    void find_file() const {
    }
};

#endif  // DEC_REP_FS_HPP_