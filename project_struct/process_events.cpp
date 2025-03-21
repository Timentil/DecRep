#include "../DecRepFS/dec_rep_fs.hpp"
#include "db_manager.hpp"

class Events {
private:
    DBManager::Manager dbManager;
    DecRepFS::DecRepFS decRepFS;

    static std::string get_name(const std::string &full_path) {
        fs::path p(full_path);
        while (!p.empty() && p.filename().empty()) {
            p = p.parent_path();
        }
        return p.filename().string();
    }

public:
    explicit Events(const std::string &connection_data)
        : dbManager(connection_data) {
    }

    void add_file(
        const std::string &local_file_path,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip
    ) {
        const std::string file_name = get_name(local_file_path);

        dbManager.add_file(
            local_file_path, file_name, DecRep_path, username, ip
        );
        decRepFS.add_file(DecRep_path, file_name);
    }

    void add_folder(
        const std::string &local_folder_path,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip
    ) {
        const std::string folder_name = get_name(local_folder_path);
        dbManager.add_folder(local_folder_path, DecRep_path, username, ip);
        decRepFS.add_folder(DecRep_path, local_folder_path);
    }

    void add_user(const std::string &username, const std::string &ip) {
        dbManager.add_into_Users(username, ip);
    }

    void update_file(
        const std::string &local_path,
        const std::string &username,
        const std::string &ip,
        std::string &new_hash,  //??
        std::size_t &new_size   //??
    ) {
        dbManager.update_file(local_path, username, ip, new_hash, new_size);
    }

    void update_local_path(
        const std::string &old_local_path,  // Может быть заменен
        const std::string &new_local_path,
        const std::string &username,
        const std::string &ip
    ) {
        dbManager.update_local_path(
            old_local_path, new_local_path, username, ip
        );
    }

    void delete_file(
        const std::string &local_path,
        const std::string &username,
        const std::string &ip
    ) {
        dbManager.delete_file(local_path, username, ip);
    }

    void delete_user(const std::string &username, const std::string &ip) {
        const std::vector<std::string> deleted_files =
            dbManager.delete_user(username, ip);
        decRepFS.delete_user_files(deleted_files);
    }

    void download() {
    }
};