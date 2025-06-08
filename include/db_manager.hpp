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

struct DbFileInfo {
    std::string DecRep_path; // тут: DecRep_path - путь до родительской директории (папки), full_DecRep_path - путь вместе с файлом
    std::string file_name;
};

class Manager {
private:
    pqxx::connection C;

    // (Вспомогательные)
    static int get_user_id(
        pqxx::work &w,
        const std::string &username
    );

    static void add_file_template(
        pqxx::work &w,
        const std::string &local_file_path,
        const std::string &file_name,
        const std::string &DecRep_path,
        const std::string &username
    );

public:
    explicit Manager(const std::string &connection_data);

    ~Manager();

    // Добавить пользователя. Если это новый пользователь "создаёт себя" (isLocal = true), также обновляется MyUsername
    // Иначе (isLocal = false) обновляется только общая таблица -- Users
    void add_user(
    const std::string &username,
    bool isLocal = false
    );

    // пользователь добавляет файл в репозиторий
    void add_file(
        const std::string &local_file_path,
        const std::string &file_name,
        const std::string &DecRep_path,
        const std::string &username
    );

    // пользователь добавляет все файлы из заданной папки в репозиторий
    void add_folder(
        const std::string &local_folder_path,
        const std::string &DecRep_path,
        const std::string &username
    );

    void rename_file(
    const std::string &DecRep_path,
    const std::string &old_file_name,
    const std::string &new_file_name
    );

    void rename_DecRep_path(
    const std::string &old_DecRep_path_name,
    const std::string &new_DecRep_path_name
    );

    void change_DecRep_path(
    const std::string &file_name,
    const std::string &old_DecRep_path,
    const std::string &new_DecRep_path
    );

    void untrack_file(const std::string &full_DecRep_path);
    // удалить файл/папку из репозитория
    void untrack_folder(const std::string &DecRep_path);

    // обновить FileOwners после того, как пользователь скачал файл
    void download_file(
        const std::string &username,
        const std::string &full_DecRep_path,
        const std::string &local_path
    );

    // удалить пользователя и все его файлы
    std::vector<std::string> delete_user(
        const std::string &username
    );

    // пользователь локально удалил файл (событие Filewatcher-a)
    std::string delete_local_file(
        const std::string &local_path,
        const std::string &username
    );

    // пользователь локально изменил путь(имя) файла (событие Filewatcher-a)
    void update_local_path(
        const std::string &old_local_path,
        const std::string &new_local_path,
        const std::string &username
    );

    // TODO: update_local_folder_path (пользователь переименовал директорию -> надо обновить файлов в ней)

    // пользователь локально изменил содержимое файла (событие Filewatcher-a)
    // (теперь он единственный владелец)
    void update_file(
        const std::string &local_path,
        const std::string &username
    );

    std::vector<DbFileInfo> get_files_info();

    bool is_users_empty(); // возвращает True, если нет юзеров

    json::value fetch_table_data(const std::string &table_name);

    json::object get_all_data();

};
} // namespace DBManager

#endif // DB_MANAGER_HPP_