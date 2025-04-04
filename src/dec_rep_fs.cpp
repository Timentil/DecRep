#include "dec_rep_fs.hpp"

namespace fs = std::filesystem;

static int INITIAL_INDENT = 0;
static int TAB = 2;

namespace DecRepFS {

Node::Node(std::string s) : name(std::move(s)) {
}

void File::print(const int level) const {
    std::cout << std::string(level, ' ') << name << '\n';
}

void Directory::print(const int level) const {
    std::cout << std::string(level, ' ') << name << '\n';
    for (auto &child : children) {
        child.second->print(level + TAB);
    }
}

DecRepFS::DecRepFS() : root("DecRep") {
}

std::vector<std::string>
DecRepFS::split_path(const std::string &path, const char delim) {
    std::vector<std::string> subdirs;
    std::stringstream ss(path);
    std::string subdir;
    while (std::getline(ss, subdir, delim)) {
        if (!subdir.empty()) {  // По идее, вернёт пустую строку, если исходная
            // начинается/заканчивается разделителем (ну или 2 разделителя
            // подряд). Думаю, можно будет убрать, когда сделаем
            // где-нибудь обработку путей
            subdirs.push_back(subdir);
        }
    }
    return subdirs;
}

void DecRepFS::add_file(const std::string &path, const std::string &file_name) {
    std::vector<std::string> subdirs = split_path(path, '/');

    Directory *current = &root;
    for (const auto &subdir : subdirs) {
        current->children.try_emplace(
            subdir, std::make_unique<Directory>(subdir)
        );
        current = dynamic_cast<Directory *>(current->children[subdir].get());
        // Тут может быть ошибка, если по такому пути/части пути до этого уже
        // был создан файл. Например, уже существует folder/subfolder/file1, а
        // хотим добавить folder/subfolder/file1/file2. По факту это может быть
        // косяком пользователя и вопрос в том, где его обрабатывать. Пусть пока
        // тут, а там разберёмся
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
        current->children.try_emplace(
            subdir, std::make_unique<Directory>(subdir)
        );
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
                add_file(folder_path.string(), file_name);
            }
        }
    }
}

void DecRepFS::delete_file(const std::string &path) {
    const std::vector<std::string> subdirs = split_path(path, '/');

    // Только если пустая строка, надо перенести отсюда
    if (subdirs.empty()) {
        throw std::runtime_error("Cannot delete nothing.");
    }
    // В отличие от добавления файла, где мы указываем путь без самого файла и
    // имя файла отдельно, удаление происходит по полному пути (ну, при
    // добавлении нужно ещё локальный путь указывать, и имя можно дёрнуть из
    // него). Собственно, поэтому тут не range-based for P.S. Можно переделать
    // эту функцию по неполному пути + имя или добавление по 2-ум полным путям
    // (получается, при добавлении можем задать имя файла)
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
        );  // Проверка, что получили файл, а не папку
    }

    current->children.erase(it);
}

void DecRepFS::delete_folder(const std::string &path) {
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
        );  // Проверка, что получили папку, а не файл
    }

    current->children.erase(it);
}

void DecRepFS::delete_user_files(const std::vector<std::string> &file_paths) {
    for (const auto &file_path : file_paths) {
        delete_file(file_path);
    }
}

void DecRepFS::print_DecRepFS() const {
    root.print(INITIAL_INDENT);
}

std::vector<std::string> DecRepFS::find(
    const std::string &name,
    const Node *node,
    const std::string &curr_path
) const {
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
            std::vector<std::string> child_res =
                find(name, child.second.get(), new_path);
            res.insert(res.end(), child_res.begin(), child_res.end());
        }
    }
    return res;
}
}  // namespace DecRepFS