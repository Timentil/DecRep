#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <pqxx/pqxx>

namespace fs = std::filesystem;

namespace DBManager {

int get_user_id(
    pqxx::work &w,
    const std::string &username,
    const std::string &ip
) {
    try {
        const pqxx::result user_id = w.exec_params(
            "SELECT id FROM Users WHERE ip = $2 AND username = $1", username, ip
        );
        if (!user_id.empty()) {
            return user_id[0]["id"].as<int>();
        } else {
            throw std::runtime_error("User does not exist");
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }
}

void add_into_Users(const std::string &username, const std::string &ip) {
    try {
        pqxx::connection C(
            "dbname=base user=postgres password=1234 host=localhost"
        );
        pqxx::work w(C);

        const pqxx::result res = w.exec_params(
            "SELECT id FROM Users WHERE username = $1 AND ip = $2", username, ip
        );
        if (res.empty()) {
            w.exec_params(
                "INSERT INTO Users (ip, username, first_connection_time) "
                "VALUES ($1, $2, NOW())",
                ip, username
            );
            w.commit();
            std::cout << "User added successfully";
        } else {
            std::cout << "User already exists";
        }
    } catch (const std::exception &e) {
        std::cout << "Error" << e.what() << std::endl;
    }
}

void add_file_template(
    pqxx::work &w,
    const std::string &local_file_path,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip
) {
    fs::path p(local_file_path);

    if (fs::is_regular_file(p)) {
        std::string file_name = p.filename().string();
        std::size_t file_size = fs::file_size(p);
        std::string hash = sha256(p.string());
        // Преобразование в TIMESTAMP для вставки в таблицу. Возможно, есть
        // способ лучше
        const std::chrono::time_point now = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(now);
        std::tm local_time = *std::localtime(&current_time);
        std::ostringstream oss;
        oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
        std::string timestamp_time = oss.str();

        int author_id = get_user_id(w, username, ip);

        // Проверять ли ещё по имени файла?
        pqxx::result res =
            w.exec_params("SELECT id FROM Files WHERE file_hash = $1", hash);

        if (res.empty()) {
            pqxx::result file_added = w.exec_params(
                "INSERT INTO Files (file_name, file_size, file_hash, "
                "addition_time, last_modified, DecRep_path, author_id) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id",
                file_name, file_size, hash, timestamp_time, timestamp_time,
                DecRep_path, author_id
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

void add_file(
    const std::string &local_file_path,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip
) {
    try {
        pqxx::connection C(
            "dbname=base user=postgres password=1234 host=localhost"
        );
        pqxx::work w(C);

        add_file_template(w, local_file_path, DecRep_path, username, ip);
        w.commit();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void add_folder(
    const std::string &local_folder_path,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip
) {
    try {
        const fs::path p(local_folder_path);
        if (!is_directory(p)) {
            std::cout << "That's not a folder";
            return;
        }

        pqxx::connection C(
            "dbname=base user=postgres password=1234 host=localhost"
        );
        pqxx::work w(C);

        for (const auto &entry : fs::recursive_directory_iterator(p)) {
            if (entry.is_regular_file()) {
                add_file_template(
                    w, entry.path().string(), DecRep_path, username, ip
                );
            }
        }
        w.commit();
    } catch (const std::exception &e) {
        std::cout << "Error " << e.what() << std::endl;
    }
}

void delete_file(
    const std::string &local_path,
    const std::string &username,
    const std::string &ip
) {
    try {
        pqxx::connection C(
            "dbname=base user=postgres password=1234 host=localhost"
        );
        pqxx::work w(C);

        int owner_id = get_user_id(w, username, ip);

        const pqxx::result res = w.exec_params(
            "SELECT file_id FROM FileOwners WHERE local_path = $1 AND owner_id "
            "= $2",
            local_path, owner_id
        );

        if (res.empty()) {
            std::cout << "You're trying to delete a file that doesn't exist\n";
            return;
        }

        int file_id = res[0]["file_id"].as<int>();

        w.exec_params(
            "DELETE FROM FileOwners WHERE local_path = $1 AND owner_id = $2 "
            "AND file_id = $3",
            local_path, owner_id, file_id
        );

        const pqxx::result res2 = w.exec_params(
            "SELECT COUNT(*) AS remain FROM FileOwners WHERE file_id = $1",
            file_id
        );

        int files_left = res2[0]["left"].as<int>();

        if (files_left == 0) {
            w.exec_params("DELETE FROM Files WHERE id = $1", file_id);
        }

        w.commit();
        std::cout << "File deleted\n";
    } catch (const std::exception &e) {
        std::cout << "Error " << e.what() << std::endl;
    }
}

}  // namespace DBManager
