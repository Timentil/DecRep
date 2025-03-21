#include "dec_rep_fs.hpp"
namespace fs = std::filesystem;

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

void DecRepFS::add_folder(
    const std::string &DecRep_path,
    const std::string &local_path
) {
    const fs::path folder_path(DecRep_path);
    std::string folder_name = folder_path.filename().string();
    const std::string parent_path = folder_path.parent_path().string();

    std::vector<std::string> subdirs = split_path(parent_path, '/');

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

    if (!current->children.contains(folder_name)) {
        current->children[folder_name] =
            std::make_unique<Directory>(folder_name);
    }

    const auto *newDir =
        dynamic_cast<Directory *>(current->children[folder_name].get());
    if (!newDir) {
        throw std::runtime_error("Failed to create directory");
    }

    if (fs::exists(local_path) && fs::is_directory(local_path)) {
        for (const auto &entry : fs::directory_iterator(local_path)) {
            if (entry.is_directory()) {
                std::string subfolder_name = entry.path().filename().string();
                fs::path subfolder_path = folder_path / subfolder_name;
                add_folder(subfolder_path.string(), entry.path().string());
            } else if (entry.is_regular_file()) {
                std::string file_name = entry.path().filename().string();
                add_file(folder_path, file_name);
            }
        }
    }
}

void DecRepFS::delete_file(const std::string &path) {
    const std::vector<std::string> subdirs = split_path(path, '/');

    if(subdirs.empty()) {
        throw std::runtime_error("File does not exist.");
    }

    Directory *current = &root;
    for (size_t i = 0; i < subdirs.size() - 1; i++) {
        auto it = current->children.find(subdirs[i]);
        if (it == current->children.end()) {
            throw std::runtime_error("File does not exist.");
        }
        current = dynamic_cast<Directory *>(it->second.get());
        if (!current) {
            throw std::runtime_error("Not a directory");
        }
    }
    current->children.erase(subdirs.back());
}

void DecRepFS::delete_user_files(const std::vector<std::string> &file_paths) {
    for (const auto &file_path : file_paths) {
        delete_file(file_path);
    }
}


void DecRepFS::print_DecRepFS() const {
    root.print(0);
}

const Node *DecRepFS::find(const std::string &full_path) const {
    if (full_path.empty()) {
        return &root;
    }
    const std::vector<std::string> subdirs = split_path(full_path, '/');

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
