#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

struct Node {
    std::string name;
    Node(const std::string &n) : name(n) {}
    virtual ~Node() = default;
    virtual void print(int level = 0) const = 0;
};

struct File : Node {
    File(const std::string &file_name) : Node(file_name) {}
    void print(int level = 0) const override {
        std::cout << std::string(level, ' ' ) << name << '\n';
    }
};

struct Directory : Node {
    Directory(const std::string &dir_name) : Node(dir_name) {}

    std::unordered_map<std::string, std::unique_ptr<Node>> children;
    void print(int level = 0) const override {
        std::cout << std::string(level, ' ' ) << name << '\n';
        for (auto &child : children) {
            child.second->print(level + 2);
        }
    }
};

struct DecRepFS {
    Directory root{"DecRep"};

    std::vector<std::string> split_path(const std::string &s, char delim) const {
        std::vector<std::string> subdirs;
        std::stringstream ss(s);
        std::string subdir;
        while (std::getline(ss, subdir, delim)) {
            if(!subdir.empty()) {
                subdirs.push_back(subdir);
            }
        }
        return subdirs;
    }

    void add_file(const std::string &path, const std::string &file_name) {

        std::vector<std::string> subdirs = split_path(path, '/');

        Directory *current = &root;
        for (const auto &subdir : subdirs) {
            if (current->children.find(subdir) == current->children.end() ) {
                current->children[subdir] = std::make_unique<Directory>(subdir);
              }
              current = dynamic_cast<Directory*>(current->children[subdir].get());
              if (!current) {
                  throw std::runtime_error("Not a directory");
              }
        }
        if (current->children.find(file_name) == current->children.end()) {
            current->children[file_name] = std::make_unique<File>(file_name);
        }
    }

    void print_DecRepFs() const {
        root.print();
    }

    const Node* find(const std::string& full_path) const {

        std::vector<std::string> subdirs = split_path(full_path, '/');

        const Directory *current = &root;
        for (size_t i = 0; i < subdirs.size(); i++) {
            auto it = current->children.find(subdirs[i]);
            if(it == current->children.end()) {
                return nullptr;
            }
            if (i == subdirs.size() - 1) {
                return it->second.get();
            }
            current = dynamic_cast<Directory*>(it->second.get());
            if (!current) {
                return nullptr;
            }
        }
        return nullptr;
    }
};

