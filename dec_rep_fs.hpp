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
    std::set<Node*> children;
    Node *parent;
};

struct DecRepFS {
    void find_file() const {
    }
};

// DecRep/1/2/3;

#endif  // DEC_REP_FS_HPP_