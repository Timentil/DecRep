#include "dec_rep_fs.hpp"

Node::Node(std::string n) : name(std::move(n)) {
}

Node::~Node() = default;

File::File(const std::string &file_name) : Node(file_name) {
}

void File::print(const int level) const {
    std::cout << std::string(level, ' ') << name << '\n';
}

Directory::Directory(const std::string &dir_name) : Node(dir_name) {
}

void Directory::print(const int level) const {
    std::cout << std::string(level, ' ') << name << '\n';
    for (auto &child : children) {
        child.second->print(level + 2);
    }
}

DecRepFS::DecRepFS() : root("DecRep"){};

std::vector<std::string>
DecRepFS::split_path(const std::string &s, const char delim) {
    std::vector<std::string> subdirs;
    std::stringstream ss(s);
    std::string subdir;
    while (std::getline(ss, subdir, delim)) {
        if (!subdir.empty()) {
            subdirs.push_back(subdir);
        }
    }
    return subdirs;
}

void DecRepFS::add_file(const std::string &path, const std::string &file_name) {
    std::vector<std::string> subdirs = split_path(path, '/');

    Directory *current = &root;
    for (const auto &subdir : subdirs) {
        if (!current->children.contains(subdir)) {
            current->children[subdir] = std::make_unique<Directory>(subdir);
        }
        current = dynamic_cast<Directory *>(current->children[subdir].get());
        if (!current) {
            throw std::runtime_error("Not a directory");
        }
    }
    if (!current->children.contains(file_name)) {
        current->children[file_name] = std::make_unique<File>(file_name);
    }
}

void DecRepFS::print_DecRepFS() const {
    root.print(0);
}

const Node *DecRepFS::find(const std::string &full_path) const {
    if (full_path.empty()) {
        return &root;
    }
    std::vector<std::string> subdirs = split_path(full_path, '/');

    const Directory *current = &root;
    for (size_t i = 0; i < subdirs.size(); i++) {
        auto it = current->children.find(subdirs[i]);
        if (it == current->children.end()) {
            return nullptr;
        }
        if (i == subdirs.size() - 1) {
            return it->second.get();
        }
        current = dynamic_cast<Directory *>(it->second.get());
        if (!current) {
            return nullptr;
        }
    }
    return nullptr;
}
