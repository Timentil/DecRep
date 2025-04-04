#ifndef DB_MANAGER_HPP_
#define DB_MANAGER_HPP_

#include <filesystem>
#include <iostream>
#include <pqxx/pqxx>
#include <string>

namespace fs = std::filesystem;

namespace DBManager {
class Manager {
private:
    pqxx::connection C;

public:
    explicit Manager(const std::string &connection_data);

    ~Manager();

    static int get_user_id(
        pqxx::work &w,
        const std::string &ip,
        const std::string &username
    );

    void add_into_Users(const std::string &username, const std::string &ip);

    static void add_file_template(
        pqxx::work &w,
        const std::string &local_file_path,
        const std::string &file_name,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip
    );

    void add_file(
        const std::string &local_file_path,
        const std::string &file_name,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip
    );

    void add_folder(
        const std::string &local_folder_path,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip
    );

    std::string delete_local_file(
        const std::string &local_path,
        const std::string &username,
        const std::string &ip
    );

    void untrack_file(const std::string &full_DecRep_path);

    void untrack_folder(const std::string &DecRep_path);

    void update_local_path(
        const std::string &old_local_path,  // Может быть заменен
        const std::string &new_local_path,
        const std::string &username,
        const std::string &ip
    );

    void update_file(
        const std::string &local_path,
        const std::string &username,
        const std::string &ip,
        std::string &new_hash,  // ???
        std::size_t &new_size   // ???
    );

    std::vector<std::string>
    delete_user(const std::string &username, const std::string &ip);
};
}  // namespace DBManager

#endif  // DB_MANAGER_HPP_