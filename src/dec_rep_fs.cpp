#include "dec_rep_fs.hpp"

namespace fs = std::filesystem;

static int INITIAL_INDENT = 0;
static int TAB = 2;

namespace DecRepFS {

Node::Node(std::string s)
    : name(std::move(s))
{
}

void File::print(const int level) const
{
    std::cout << std::string(level, ' ') << name << '\n';
}

void Directory::print(const int level) const
{
    std::cout << std::string(level, ' ') << name << '\n';
    for (auto &child : children) {
        child.second->print(level + TAB);
    }
}

FS::FS()
    : root("DecRep")
{
}

std::vector<std::string>
FS::split_path(const std::string &path, const char delim)
{
    std::vector<std::string> subdirs;
    std::stringstream ss(path);
    std::string subdir;
    while (std::getline(ss, subdir, delim)) {
        if (!subdir.empty()) {
            subdirs.push_back(subdir);
        }
    }
    return subdirs;
}

void FS::add_file(const std::string &path, const std::string &file_name)
{
    std::vector<std::string> subdirs = split_path(path, '/');

    Directory *current = &root;
    for (const auto &subdir : subdirs) {
        current->children.try_emplace(
            subdir, std::make_unique<Directory>(subdir)
        );
        current = dynamic_cast<Directory *>(current->children[subdir].get());
        if (!current) {
            throw std::runtime_error("Not a directory");
        }
    }
    if (!current->children.contains(file_name)) {
        current->children[file_name] = std::make_unique<File>(file_name);
    }
}

void FS::add_folder(
    const std::string &DecRep_path,
    const std::string &local_path
)
{
    const fs::path folder_path(DecRep_path);
    std::string folder_name = folder_path.filename().string();
    const std::string parent_path = folder_path.parent_path().string();

    std::vector<std::string> subdirs = split_path(parent_path, '/');

    Directory *current = &root;
    for (const auto &subdir : subdirs) {
        current->children.try_emplace(
            subdir, std::make_unique<Directory>(subdir)
        );
        current = dynamic_cast<Directory *>(current->children[subdir].get());
        if (!current) {
            throw std::runtime_error("Not a directory");
        }
    }

    if (!current->children.contains(folder_name)) {
        current->children[folder_name] = std::make_unique<Directory>(folder_name);
    }

    const auto *newDir = dynamic_cast<Directory *>(current->children[folder_name].get());
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
                add_file(folder_path.string(), file_name);
            }
        }
    }
}

void FS::delete_file(const std::string &path)
{
    const std::vector<std::string> subdirs = split_path(path, '/');

    if (subdirs.empty()) {
        throw std::runtime_error("Cannot delete nothing.");
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

    const auto it = current->children.find(subdirs.back());
    if (it == current->children.end()) {
        throw std::runtime_error("File does not exist.");
    }

    if (dynamic_cast<File *>(it->second.get()) == nullptr) {
        throw std::runtime_error("Not a file"
        );
    }

    current->children.erase(it);
}

void FS::delete_folder(const std::string &path)
{
    const std::vector<std::string> subdirs = split_path(path, '/');

    Directory *current = &root;
    for (size_t i = 0; i < subdirs.size() - 1; i++) {
        auto it = current->children.find(subdirs[i]);
        if (it == current->children.end()) {
            throw std::runtime_error("Folder does not exist.");
        }
        current = dynamic_cast<Directory *>(it->second.get());
        if (!current) {
            throw std::runtime_error("Not a directory");
        }
    }

    const auto it = current->children.find(subdirs.back());
    if (it == current->children.end()) {
        throw std::runtime_error("Folder does not exist.");
    }

    if (dynamic_cast<Directory *>(it->second.get()) == nullptr) {
        throw std::runtime_error("Not a directory"
        );
    }

    current->children.erase(it);
}

void FS::delete_user_files(const std::vector<std::string> &file_paths)
{
    for (const auto &file_path : file_paths) {
        delete_file(file_path);
    }
}

void FS::rename_file(
    const std::string &DecRep_path,
    const std::string &old_file_name,
    const std::string &new_file_name
)
{
    std::vector<std::string> subdirs = split_path(DecRep_path, '/');

    Directory *current = &root;
    for (const auto &subdir : subdirs) {
        auto it = current->children.find(subdir);
        if (it == current->children.end()) {
            throw std::runtime_error("Directory does not exist: " + subdir);
        }
        current = dynamic_cast<Directory *>(it->second.get());
        if (!current) {
            throw std::runtime_error(subdir + " is not a directory");
        }
    }

    const auto file_it = current->children.find(old_file_name);
    if (file_it == current->children.end()) {
        throw std::runtime_error("File does not exist: " + old_file_name);
    }

    if (dynamic_cast<File *>(file_it->second.get()) == nullptr) {
        throw std::runtime_error(old_file_name + " is not a file");
    }

    if (current->children.contains(new_file_name)) {
        throw std::runtime_error(new_file_name + " already exists");
    }

    auto node_ptr = std::move(file_it->second);
    node_ptr->name = new_file_name;
    current->children.erase(file_it);
    current->children.emplace(
        new_file_name,
        std::move(node_ptr)
    );
}

void FS::rename_folder(
    const std::string &old_DecRep_path_name,
    const std::string &new_DecRep_path_name
)
{
    if (new_DecRep_path_name == old_DecRep_path_name || new_DecRep_path_name.rfind(old_DecRep_path_name + "/", 0) == 0) {
        throw std::runtime_error(
            "Don't move folder into itself or its subdirectory, silly!!"
        );
    }

    auto old_subdirs = split_path(old_DecRep_path_name, '/');
    if (old_subdirs.empty()) {
        throw std::runtime_error("Empty path");
    }
    std::vector<std::string> old_parent_subdirs(
        old_subdirs.begin(), old_subdirs.end() - 1
    );
    const std::string &old_folder_name = old_subdirs.back();

    Directory *old_parent = &root;
    for (const auto &p : old_parent_subdirs) {
        auto it = old_parent->children.find(p);
        if (it == old_parent->children.end()) {
            throw std::runtime_error("Wrong path");
        }
        old_parent = dynamic_cast<Directory *>(it->second.get());
        if (!old_parent) {
            throw std::runtime_error(p + " in old path is not a directory");
        }
    }

    auto old_it = old_parent->children.find(old_folder_name);
    if (old_it == old_parent->children.end()) {
        throw std::runtime_error("Directory to rename does not exist: " + old_folder_name);
    }

    if (dynamic_cast<Directory *>(old_it->second.get()) == nullptr) {
        throw std::runtime_error(old_folder_name + " is not a directory");
    }

    auto dir_ptr = std::move(old_it->second);
    old_parent->children.erase(old_it);

    auto new_subdirs = split_path(new_DecRep_path_name, '/');
    if (new_subdirs.empty()) {
        throw std::runtime_error("New path is empty");
    }
    std::vector<std::string> new_parent_subdirs(
        new_subdirs.begin(), new_subdirs.end() - 1
    );
    const std::string &new_folder_name = new_subdirs.back();

    Directory *new_parent = &root;
    for (const auto &p : new_parent_subdirs) {
        auto [it, inserted] = new_parent->children.try_emplace(
            p,
            std::make_unique<Directory>(p)
        );
        new_parent = dynamic_cast<Directory *>(it->second.get());
        if (!new_parent) {
            throw std::runtime_error(p + " in new path is not a directory");
        }
    }

    if (new_parent->children.contains(new_folder_name)) {
        throw std::runtime_error(
            new_folder_name + " already exists"
        );
    }

    auto *moved_dir = dynamic_cast<Directory *>(dir_ptr.get());
    moved_dir->name = new_folder_name;
    new_parent->children.emplace(new_folder_name, std::move(dir_ptr));
}

void FS::change_path(
    const std::string &file_name,
    const std::string &old_DecRep_path,
    const std::string &new_DecRep_path
)
{

    auto old_subdirs = split_path(old_DecRep_path, '/');
    Directory *old_parent = &root;
    for (const auto &p : old_subdirs) {
        auto it = old_parent->children.find(p);
        if (it == old_parent->children.end()) {
            throw std::runtime_error("Old directory does not exist: " + p);
        }
        old_parent = dynamic_cast<Directory *>(it->second.get());
        if (!old_parent) {
            throw std::runtime_error(p + " in old path is not a directory");
        }
    }

    const auto file_it = old_parent->children.find(file_name);
    if (file_it == old_parent->children.end()) {
        throw std::runtime_error("File does not exist in old path: " + file_name);
    }

    if (dynamic_cast<File *>(file_it->second.get()) == nullptr) {
        throw std::runtime_error(file_name + " is not a file");
    }
    auto file_ptr = std::move(file_it->second);
    old_parent->children.erase(file_it);

    auto new_subdirs = split_path(new_DecRep_path, '/');
    Directory *new_parent = &root;
    for (const auto &p : new_subdirs) {
        auto [it, inserted] = new_parent->children.try_emplace(
            p,
            std::make_unique<Directory>(p)
        );
        new_parent = dynamic_cast<Directory *>(it->second.get());
        if (!new_parent) {
            throw std::runtime_error(p + " in new path is not a directory");
        }
    }

    if (new_parent->children.contains(file_name)) {
        throw std::runtime_error(
            old_DecRep_path + " already has a file named: " + file_name
        );
    }

    new_parent->children.emplace(file_name, std::move(file_ptr));
}

void FS::print_DecRepFS() const
{
    root.print(INITIAL_INDENT);
}

std::vector<std::string> FS::find_path(
    const std::string &name,
    const Node *node,
    const std::string &curr_path
) const
{
    if (node == nullptr) {
        node = &root;
    }

    std::vector<std::string> res;

    std::string new_path;
    if (curr_path.empty()) {
        new_path = node->name;
    } else {
        new_path = curr_path + "/" + node->name;
    }

    if (node->name == name) {
        res.push_back(new_path);
    }

    if (const auto *dir = dynamic_cast<const Directory *>(node)) {
        for (const auto &child : dir->children) {
            std::vector<std::string> child_res = find_path(name, child.second.get(), new_path);
            res.insert(res.end(), child_res.begin(), child_res.end());
        }
    }
    return res;
}
} // namespace DecRepFS