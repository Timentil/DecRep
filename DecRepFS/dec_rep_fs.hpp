#ifndef DEC_REP_FS_HPP
#define DEC_REP_FS_HPP

#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct Node {
    std::string name;
    explicit Node(std::string n);
    virtual ~Node();

    virtual void print(int level) const = 0;
};

struct File final : Node {
    explicit File(const std::string &file_name);

    void print(int level) const override;
};

struct Directory final : Node {
    explicit Directory(const std::string &dir_name);
    std::unordered_map<std::string, std::unique_ptr<Node>> children;

    void print(int level) const override;
};

struct DecRepFS {
    Directory root;
    DecRepFS();

    static std::vector<std::string>
    split_path(const std::string &s, char delim);

    void add_file(const std::string &path, const std::string &file_name);

    void
    add_folder(const std::string &DecRep_path, const std::string &local_path);

    void delete_file(const std::string &path);

    void delete_user_files(const std::vector<std::string> &file_paths);

    void print_DecRepFS() const;

    const Node *find(const std::string &full_path) const;
};

#endif  // DEC_REP_FS_HPP