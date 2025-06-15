#ifndef DEC_REP_FS_HPP_
#define DEC_REP_FS_HPP_

#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace DecRepFS {

struct Node {
    std::string name;
    explicit Node(std::string s);
    virtual ~Node() = default;

    virtual void print(int level) const = 0;
};

struct File final : Node {
    using Node::Node;

    void print(int level) const override;
};

struct Directory final : Node {
    using Node::Node;
    std::unordered_map<std::string, std::unique_ptr<Node>> children;

    void print(int level) const override;
};

struct FS {
private:
    Directory root;

    static std::vector<std::string>
    split_path(const std::string &path, char delim);

public:
    FS();

    void add_file(const std::string &path, const std::string &file_name);

    void
    add_folder(const std::string &DecRep_path, const std::string &local_path);

    void delete_file(const std::string &path);

    void delete_folder(const std::string &path);

    void delete_user_files(const std::vector<std::string> &file_paths);

    void rename_file(const std::string &DecRep_path, const std::string &old_file_name, const std::string &new_file_name);

    void rename_folder(
        const std::string &old_DecRep_path_name,
        const std::string &new_DecRep_path_name
    );

    void change_path(
        const std::string &file_name,
        const std::string &old_DecRep_path,
        const std::string &new_DecRep_path
    );

    void print_DecRepFS() const;

    std::vector<std::string> find_path(
        const std::string &name,
        const Node *node = nullptr,
        const std::string &curr_path = ""
    ) const;
};
} // namespace DecRepFS

#endif // DEC_REP_FS_HPP_