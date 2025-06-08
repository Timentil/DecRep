#include "../include/db_manager.hpp"

namespace fs = std::filesystem;

namespace DBManager {

Manager::Manager(const std::string &connection_data)
    : C(connection_data)
{
    if (!C.is_open()) {
        std::cerr << "Error opening database connection" << std::endl;
    }
}

Manager::~Manager()
{
    if (C.is_open()) {
        C.disconnect();
    }
}

int Manager::get_user_id(
    pqxx::work &w,
    const std::string &username
)
{
    const pqxx::result user_id = w.exec_params(
        "SELECT id FROM Users WHERE username = $1",
        username
    );
    if (!user_id.empty()) {
        return user_id[0]["id"].as<int>();
    } else {
        throw std::runtime_error("User does not exist");
    }
}

void Manager::add_user(
    const std::string &username,
    bool isLocal
)
{
    pqxx::work w(C);

    const pqxx::result res = w.exec_params(
        "SELECT 1 FROM Users WHERE username = $1",
        username
    );

    if (!res.empty()) {
        std::cout << "Username already exists, please choose another\n";
        return;
    }

    w.exec_params(
        "INSERT INTO Users(username, first_connection_time) "
        "VALUES($1, NOW()) ",
        username
    );

    if (isLocal) {
        w.exec_params(
            "INSERT INTO MyUsername(username) VALUES($1) "
            "ON CONFLICT(username) DO UPDATE "
            "  SET username = EXCLUDED.username",
            username
        );
        std::cout << "Your username is " << username << "\n";
    }

    w.commit();
    std::cout << "User added\n";
}

void Manager::add_file_template(
    pqxx::work &w,
    const std::string &local_file_path,
    const std::string &file_name,
    const std::string &DecRep_path,
    const std::string &username
)
{
    fs::path p(local_file_path);

    if (fs::is_regular_file(p)) {
        auto file_size = fs::file_size(p);

        int author_id = get_user_id(w, username);

        const pqxx::result res = w.exec_params(
            "SELECT id FROM Files WHERE file_name = $1 AND DecRep_path = $2",
            file_name, DecRep_path
        );

        if (res.empty()) {
            const pqxx::result file_added = w.exec_params(
                "INSERT INTO Files (file_name, file_size, "
                "addition_time, last_modified, DecRep_path, author_id) "
                "VALUES ($1, $2, NOW(), NOW(), $3, $4) RETURNING id",
                file_name, file_size, DecRep_path, author_id
            );

            int file_id = file_added[0]["id"].as<int>();

            w.exec_params(
                "INSERT INTO FileOwners (owner_id, file_id, local_path) "
                "VALUES ($1, $2, $3)",
                author_id, file_id, local_file_path
            );

            std::cout << "File added\n";
        } else {
            std::cout << "Already exists\n";
        }
    }
}

void Manager::add_file(
    const std::string &local_file_path,
    const std::string &file_name,
    const std::string &DecRep_path,
    const std::string &username
)
{
    pqxx::work w(C);

    add_file_template(
        w, local_file_path, file_name, DecRep_path, username
    );
    w.commit();
}

void Manager::add_folder(
    const std::string &local_folder_path,
    const std::string &DecRep_path,
    const std::string &username
)
{
    const fs::path p(local_folder_path);
    if (!is_directory(p)) {
        std::cout << "That's not a folder\n";
        return;
    }

    pqxx::work w(C);

    for (const auto &entry : fs::recursive_directory_iterator(p)) {
        if (entry.is_regular_file()) {
            std::string file_name = entry.path().filename().string();
            add_file_template(
                w, entry.path().string(), file_name, DecRep_path, username
            );
        }
    }
    w.commit();
    std::cout << "Folder added\n";
}

void Manager::rename_file(
    const std::string &DecRep_path,
    const std::string &old_file_name,
    const std::string &new_file_name
)
{
    pqxx::work w(C);
    w.exec_params(
        "UPDATE Files SET file_name = $1 "
        "WHERE DecRep_path = $2 AND file_name = $3",
        new_file_name, DecRep_path, old_file_name
    );
    w.commit();
    std::cout << "File renamed successfully \n";
}

void Manager::change_DecRep_path(
    const std::string &file_name,
    const std::string &old_DecRep_path,
    const std::string &new_DecRep_path
)
{
    pqxx::work w(C);
    w.exec_params(
        "UPDATE Files SET DecRep_path = $1 "
        "WHERE file_name = $2 AND DecRep_path = $3",
        new_DecRep_path, file_name, old_DecRep_path
    );
    w.commit();
    std::cout << "Path changed for file '" << file_name << "'\n";
}

void Manager::rename_DecRep_path(
    const std::string &old_DecRep_path_name,
    const std::string &new_DecRep_path_name
)
{
    pqxx::work w(C);
    w.exec_params(
        "UPDATE Files SET DecRep_path = $1 "
        "WHERE DecRep_path = $2",
        new_DecRep_path_name, old_DecRep_path_name
    );
    w.commit();
    std::cout << "DecRep_path renamed from '" << old_DecRep_path_name
              << "' to '" << new_DecRep_path_name << "\n";
}

std::string Manager::delete_local_file(
    const std::string &local_path,
    const std::string &username
)
{
    pqxx::work w(C);
    std::string delete_full_path = "";

    int owner_id = get_user_id(w, username);

    const pqxx::result res = w.exec_params(
        "SELECT file_id FROM FileOwners WHERE local_path = $1 AND "
        "owner_id = $2",
        local_path, owner_id
    );

    if (res.empty()) {
        std::cout << "File doesn't exist\n";
        return delete_full_path;
    }

    int file_id = res[0]["file_id"].as<int>();

    w.exec_params(
        "DELETE FROM FileOwners WHERE file_id = $1",
        file_id
    );

    const pqxx::result res2 = w.exec_params(
        "SELECT COUNT(*) AS remain FROM FileOwners WHERE file_id = $1",
        file_id
    );

    int owners_left = res2[0]["remain"].as<int>();

    if (owners_left == 0) {
        const pqxx::result untrack_file = w.exec_params(
            "DELETE FROM Files "
            "WHERE id = $1 "
            "RETURNING DecRep_path, file_name",
            file_id
        );
        auto file_path = untrack_file[0]["DecRep_path"].as<std::string>();
        auto file_name = untrack_file[0]["file_name"].as<std::string>();
        fs::path full_path = fs::path(file_path) / file_name;
        delete_full_path = full_path.string();

        w.exec_params("DELETE FROM Files WHERE id = $1", file_id);
    }
    w.commit();
    std::cout << "File " << delete_full_path << " deleted from DecRep\n";
    return delete_full_path;
}

void Manager::untrack_file(const std::string &full_DecRep_path)
{
    pqxx::work w(C);

    fs::path p(full_DecRep_path);
    std::string file_name = p.filename().string();
    std::string file_path = p.parent_path().string();

    const pqxx::result res = w.exec_params(
        "SELECT id FROM Files WHERE file_name = $1 AND DecRep_path = $2",
        file_name, file_path
    );

    if (res.empty()) {
        std::cout << "File doesn't exist\n";
        return;
    }

    int file_id = res[0]["id"].as<int>();

    w.exec_params("DELETE FROM FileOwners WHERE file_id = $1", file_id);
    w.exec_params("DELETE FROM Files WHERE id = $1", file_id);

    w.commit();
    std::cout << "File deleted\n";
}

void Manager::untrack_folder(const std::string &DecRep_path)
{
    pqxx::work w(C);

    const pqxx::result res = w.exec_params(
        "SELECT id FROM Files WHERE DecRep_path = $1", DecRep_path
    );

    if (res.empty()) {
        return;
    }

    for (const auto &file : res) {
        int file_id = file["id"].as<int>();
        w.exec_params("DELETE FROM FileOwners WHERE file_id = $1", file_id);
    }

    w.exec_params("DELETE FROM Files WHERE DecRep_path = $1", DecRep_path);

    w.commit();
    std::cout << "Folder deleted\n";
}

void Manager::download_file(const std::string &username, const std::string &full_DecRep_path, const std::string &local_path)
{
    pqxx::work w(C);

    fs::path p(full_DecRep_path);
    std::string file_name = p.filename().string();
    std::string file_path = p.parent_path().string();

    const pqxx::result res = w.exec_params(
        "SELECT id FROM Files WHERE file_name = $1 AND DecRep_path = $2",
        file_name, file_path
    );
    int file_id = res[0]["id"].as<int>();
    int user_id = get_user_id(w, username);

    w.exec_params(
        "INSERT INTO FileOwners (owner_id, file_id, local_path) VALUES ($1, $2, $3)",
        user_id, file_id, local_path
    );
    w.commit();
}

void Manager::update_local_path(
    const std::string &old_local_path,
    const std::string &new_local_path,
    const std::string &username
)
{
    pqxx::work w(C);

    int owner_id = get_user_id(w, username);

    w.exec_params(
        "UPDATE FileOwners SET local_path = $1 WHERE local_path = $2 AND owner_id = $3",
        new_local_path, old_local_path, owner_id
    );
    w.commit();

    std::cout << "Local path updated\n";
}

void Manager::update_file(
    const std::string &local_path,
    const std::string &username
)
{
    pqxx::work w(C);

    int owner_id = get_user_id(w, username);

    const pqxx::result res = w.exec_params(
        "SELECT file_id FROM FileOwners WHERE local_path = $1 AND owner_id = $2",
        local_path, owner_id
    );
    if (res.empty()) {
        std::cout << "File doesn't exist\n";
    }
    int file_id = res[0]["file_id"].as<int>();

    auto new_size = fs::file_size(local_path); // в байтах

    w.exec_params(
        "UPDATE Files SET file_size = $1, last_modified = NOW() WHERE id = $2",
        new_size, file_id
    );

    w.exec_params(
        "DELETE FROM FileOwners WHERE file_id = $1 AND owner_id <> $2",
        file_id, owner_id
    );

    w.commit();

    std::cout << "File updated\n";
}

std::vector<std::string> Manager::delete_user(
    const std::string &username
)
{
    pqxx::work w(C);
    std::vector<std::string> deleted;

    int user_id = get_user_id(w, username);

    const pqxx::result deleted_files = w.exec_params(
        "DELETE FROM FileOwners WHERE owner_id = $1 RETURNING file_id, DecRep_path",
        user_id
    );

    if (!deleted_files.empty()) {
        for (const auto &file : deleted_files) {
            int file_id = file["file_id"].as<int>();

            const pqxx::result file_owners = w.exec_params(
                "SELECT COUNT(*) AS remain FROM FileOwners WHERE file_id = $1",
                file_id
            );

            int owners_left = file_owners[0]["remain"].as<int>();

            if (owners_left == 0) {
                deleted.push_back(file["DecRep_path"].as<std::string>());
                w.exec_params("DELETE FROM Files WHERE id = $1", file_id);
            }
        }
    }

    w.exec_params("DELETE FROM Users WHERE id = $1", user_id);
    w.commit();

    std::cout << "User and " << deleted.size() << " files deleted\n";
    return deleted;
}

std::vector<DbFileInfo> Manager::get_files_info()
{
    pqxx::work w(C);

    std::vector<DbFileInfo> files;

    pqxx::result R = w.exec(
        "SELECT DecRep_path, file_name FROM Files"
    );

    for (const pqxx::row &row : R) {
        DbFileInfo info;
        info.DecRep_path = row["DecRep_path"].as<std::string>();
        info.file_name = row["file_name"].as<std::string>();

        files.push_back(info);
    }

    return files;
}

bool Manager::is_users_empty()
{
    pqxx::work w(C);

    pqxx::result result = w.exec(
        "SELECT EXISTS (SELECT 1 FROM " + w.quote_name("users") + ")"
    );

    return !result[0][0].as<bool>();
}

json::value Manager::fetch_table_data(const std::string &table_name)
{
    json::array table_json;
    pqxx::work w(C);
    pqxx::result result = w.exec("SELECT * FROM " + w.quote_name(table_name));

    for (const auto &row : result) {
        json::array row_json;
        for (const auto &field : row) {
            row_json.push_back(json::value(field.c_str()));
        }
        table_json.push_back(row_json);
    }
    w.commit();

    return table_json;
}

json::object Manager::get_all_data()
{
    json::object response_json;

    response_json["users"] = fetch_table_data("users");
    response_json["files"] = fetch_table_data("files");
    response_json["filesowners"] = fetch_table_data("filesowners");

    return response_json;
}
} // namespace DBManager
