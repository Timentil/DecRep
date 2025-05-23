#include "db_manager.hpp"
#include "sha256_stub.hpp"

namespace fs = std::filesystem;

namespace DBManager {

Manager::Manager(const std::string &connection_data) : C(connection_data) {
    if (!C.is_open()) {
        std::cerr << "Error opening database connection" << std::endl;
    }
}

Manager::~Manager() {
    if (C.is_open()) {
        C.disconnect();
    }
}

int Manager::get_user_id(
    pqxx::work &w,
    const std::string &ip,
    const std::string &port,
    const std::string &username
) {
    const pqxx::result user_id = w.exec_params(
        "SELECT id FROM Users WHERE ip = $1 AND port = $2 AND username = $3",
        ip, port, username
    );
    if (!user_id.empty()) {
        return user_id[0]["id"].as<int>();
    } else {
        throw std::runtime_error("User does not exist");
    }
}

void Manager::insert_into_Users(
    const std::string &username,
    const std::string &ip,
    const std::string &port,
    const std::string &first_conn_time = "NOW()"
) {
    pqxx::work w(C);

    const pqxx::result res = w.exec_params(
        "SELECT id FROM Users WHERE ip = $1 AND port = $2 AND username = $3",
        ip, port, username
    );
    if (res.empty()) {
        w.exec_params(
            "INSERT INTO Users (ip, port, username, first_connection_time) "
            "VALUES ($1, $2, $3, $4)",
            ip, port, username, first_conn_time
        );
        w.commit();
        std::cout << "User added successfully\n";
    } else {
        std::cout << "User already exists\n";
    }
}

void Manager::insert_into_Files(
    const std::string &file_name,
    const std::string &file_size,
    const std::string &file_hash,
    const std::string &addition_time,
    const std::string &last_modified,
    const std::string &DecRep_path,
    const std::string &author_id
) {
    pqxx::work w(C);

    w.exec_params(
        "INSERT INTO Files (file_name, file_size, file_hash, addition_time, last_modified, DecRep_path, author_id) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7)",
        file_name, file_size, file_hash, addition_time, last_modified, DecRep_path, author_id
    );
    w.commit();
}

void Manager::insert_into_FileOwners(
    const std::string &owner_id,
    const std::string &file_id,
    const std::string &local_path
) {
    pqxx::work w(C);

    w.exec_params(
        "INSERT INTO FileOwners (owner_id, file_id, local_path) "
        "VALUES ($1, $2, $3)",
        owner_id, file_id, local_path
    );
    w.commit();
}

void Manager::add_file_template(
    pqxx::work &w,
    const std::string &local_file_path,
    const std::string &file_name,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip,
    const std::string &port
) {
    fs::path p(local_file_path);

    if (fs::is_regular_file(p)) {
        std::size_t file_size = fs::file_size(p);
        // Вероятно, должно передаваться в параметры
        std::string hash = sha256(p.string());

        int author_id = get_user_id(w, ip, username, port);

        // Проверять ли ещё по имени файла?
        const pqxx::result res =
            w.exec_params("SELECT id FROM Files WHERE file_hash = $1", hash);

        if (res.empty()) {
            const pqxx::result file_added = w.exec_params(
                "INSERT INTO Files (file_name, file_size, file_hash, "
                "addition_time, last_modified, DecRep_path, author_id) "
                "VALUES ($1, $2, $3, NOW(), NOW(), $4, $5) RETURNING id",
                file_name, file_size, hash, DecRep_path, author_id
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
    const std::string &username,
    const std::string &ip,
    const std::string &port
) {
    pqxx::work w(C);

    add_file_template(
        w, local_file_path, file_name, DecRep_path, username, ip, port
    );
    w.commit();
}

void Manager::add_folder(
    const std::string &local_folder_path,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip,
    const std::string &port
) {
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
                w, entry.path().string(), file_name, DecRep_path, username,
                ip, port
            );
        }
    }
    w.commit();
}

std::string Manager::delete_local_file(
    const std::string &local_path,
    const std::string &username,
    const std::string &ip,
    const std::string &port
) {
    pqxx::work w(C);
    std::string delete_full_path = "";

    int owner_id = get_user_id(w, ip, port, username);

    const pqxx::result res = w.exec_params(
        "SELECT file_id FROM FileOwners WHERE local_path = $1 AND "
        "owner_id "
        "= $2",
        local_path, owner_id
    );

    if (res.empty()) {
        std::cout << "File doesn't exist\n";
        return local_path;
    }

    int file_id = res[0]["file_id"].as<int>();

    w.exec_params(
        "DELETE FROM FileOwners WHERE local_path = $1 AND owner_id = "
        "$2 "
        "AND file_id = $3",
        local_path, owner_id, file_id
    );

    const pqxx::result res2 = w.exec_params(
        "SELECT COUNT(*) AS remain FROM FileOwners WHERE file_id = $1",
        file_id
    );

    int owners_left = res2[0]["remain"].as<int>();

    if (owners_left == 0) {
        const pqxx::result untrack_file = w.exec_params(
            "SELECT DecRep_path, file_name FROM Files WHERE file_id = $1",
            file_id
        );
        auto file_path = untrack_file[0]["DecRep_path"].as<std::string>();
        auto file_name = untrack_file[0]["file_name"].as<std::string>();
        fs::path full_path = fs::path(file_path) / file_name;
        delete_full_path = full_path.string();

        w.exec_params("DELETE FROM Files WHERE id = $1", file_id);
    }
    w.commit();
    std::cout << "File deleted\n";
    return delete_full_path;
}

void Manager::untrack_file(const std::string &full_DecRep_path) {
    pqxx::work w(C);

    fs::path p(full_DecRep_path);
    std::string file_name = p.filename().string();
    std::string file_path = p.parent_path().string();

    const pqxx::result res = w.exec_params(
        "SELECT id FROM Files WHERE File_name = $1 AND DecRep_path = $2",
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

void Manager::untrack_folder(const std::string &DecRep_path) {
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

void Manager::update_local_path(
    const std::string &old_local_path,  // Может быть заменен
    const std::string &new_local_path,
    const std::string &username,
    const std::string &ip,
    const std::string &port
) {
    pqxx::work w(C);

    int owner_id = get_user_id(w, ip, port, username);

    w.exec_params(
        "UPDATE FileOwners SET local_path = $1 WHERE local_path = $2 "
        "AND "
        "owner_id = $3",
        new_local_path, old_local_path, owner_id
    );
    w.commit();

    std::cout << "Local path updated\n";
}

void Manager::update_file(
    const std::string &local_path,
    const std::string &username,
    const std::string &ip,
    const std::string &port,
    std::string &new_hash,
    std::size_t &new_size
) {
    pqxx::work w(C);

    int owner_id = get_user_id(w, ip, port, username);

    const pqxx::result res = w.exec_params(
        "SELECT file_id FROM FileOwners WHERE local_path = $1 AND "
        "owner_id "
        "= $2",
        local_path, owner_id
    );
    if (res.empty()) {
        std::cout << "File doesn't exist\n";
    }
    int file_id = res[0]["file_id"].as<int>();

    w.exec_params(
        "UPDATE Files SET file_hash = $1, file_size = $2, "
        "last_modified = "
        "NOW() WHERE id = $3",
        new_hash, new_size, file_id
    );
    w.commit();

    std::cout << "File updated\n";
}

std::vector<std::string> Manager::delete_user(
    const std::string &username,
    const std::string &ip,
    const std::string &port
) {
    pqxx::work w(C);
    std::vector<std::string> deleted;

    int user_id = get_user_id(w, ip, port, username);

    const pqxx::result deleted_files = w.exec_params(
        "DELETE FROM FileOwners WHERE owner_id = $1 RETURNING file_id, "
        "DecRep_path",
        user_id
    );

    for (const auto &file : deleted_files) {
        int file_id = file["file_id"].as<int>();

        const pqxx::result file_owners = w.exec_params(
            "SELECT COUNT(*) AS remain FROM FileOwners WHERE file_id = "
            "$1",
            file_id
        );

        int owners_left = file_owners[0]["remain"].as<int>();

        if (owners_left == 0) {
            deleted.push_back(file["DecRep_path"].as<std::string>());
            w.exec_params("DELETE FROM Files WHERE id = $1", file_id);
        }
    }
    w.exec_params("DELETE FROM Users WHERE id = $1", user_id);
    w.commit();

    std::cout << "User deleted\n";
    return deleted;
}

bool Manager::is_users_empty() {
    pqxx::work w(C);

    pqxx::result result = w.exec(
        "SELECT EXISTS (SELECT 1 FROM " + w.quote_name("users") + ")"
    );

    return result[0][0].as<bool>();
}

json::value Manager::fetch_table_data(const std::string &table_name) {
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

json::object Manager::get_all_data() {
    json::object response_json;

    response_json["users"] = fetch_table_data("users");
    response_json["files"] = fetch_table_data("files");
    response_json["filesowners"] = fetch_table_data("filesowners");

    return response_json;
}
}  // namespace DBManager
