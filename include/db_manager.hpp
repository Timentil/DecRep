#ifndef DB_MANAGER_HPP_
#define DB_MANAGER_HPP_

#include <boost/json.hpp>
#include <filesystem>
#include <iostream>
#include <pqxx/pqxx>
#include <string>

namespace fs = std::filesystem;
namespace json = boost::json;

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
        const std::string &port,
        const std::string &username
    );

    void insert_into_Users(
        const std::string &username,
        const std::string &ip,
        const std::string &port,
        const std::string &first_conn_time = "NOW()"
    );

    void insert_into_Files(
        const std::string &file_name,
        const std::string &file_size,
        const std::string &file_hash,
        const std::string &addition_time,
        const std::string &last_modified,
        const std::string &DecRep_path,
        const std::string &author_id
    );

    void insert_into_FileOwners(
        const std::string &owner_id,
        const std::string &file_id,
        const std::string &local_path
    );

    static void add_file_template(
        pqxx::work &w,
        const std::string &local_file_path,
        const std::string &file_name,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void add_file(
        const std::string &local_file_path,
        const std::string &file_name,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void add_folder(
        const std::string &local_folder_path,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    std::string delete_local_file(
        const std::string &local_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void untrack_file(const std::string &full_DecRep_path);

    void untrack_folder(const std::string &DecRep_path);

    void update_local_path(
        const std::string &old_local_path, // Может быть заменен
        const std::string &new_local_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void update_file(
        const std::string &local_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port,
        std::string &new_hash, // ???
        std::size_t &new_size // ???
    );

    std::vector<std::string> delete_user(
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    bool is_users_empty();

    json::value fetch_table_data(const std::string &table_name);

    json::object get_all_data();
};
} // namespace DBManager

#endif // DB_MANAGER_HPP_