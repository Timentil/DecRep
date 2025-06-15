#include "../include/db_manager.hpp"
#include "gtest/gtest.h"
#include <exception>
#include <fstream>
#include <pqxx/pqxx>
#include <sstream>

// ВАЖНО: Перед запуском тестов убедитесь, что у вас есть
// запущенный экземпляр PostgreSQL и создана тестовая база данных.
const std::string TEST_DB_CONNECTION = "dbname=testdb user=testuser password=testpass hostaddr=127.0.0.1 port=5432";

// Вспомогательная функция для создания временного файла
void create_temp_file(const std::string &path, const std::string &content)
{
    std::ofstream ofs(path);
    ofs << content;
    ofs.close();
}

class DBManagerTest : public ::testing::Test {
protected:
    // Указатель на объект менеджера, который мы будем тестировать
    std::unique_ptr<DBManager::Manager> manager;
    // Отдельное соединение для проверки состояния БД
    std::unique_ptr<pqxx::connection> C_check;

    // Выполняется перед каждым тестом в наборе
    void SetUp() override
    {
        // --- Шаг 1: Чтение SQL файла ---
        std::ifstream sql_file("db_struct.sql");
        if (!sql_file.is_open()) {
            // Если файл не найден, тест должен упасть с ошибкой.
            throw std::runtime_error("Не удалось открыть файл db_struct.sql. "
                                     "Убедитесь, что он скопирован в директорию сборки.");
        }

        std::stringstream buffer;
        buffer << sql_file.rdbuf();
        const std::string sql_schema = buffer.str();

        // --- Шаг 2: Выполнение запроса к БД ---
        C_check = std::make_unique<pqxx::connection>(TEST_DB_CONNECTION);

        pqxx::work w_init(*C_check);
        w_init.exec("DROP TABLE IF EXISTS FileOwners, Files, MyUsername, Users CASCADE;");
        w_init.commit();

        pqxx::work w_setup(*C_check);
        // Выполняем SQL-схему, прочитанную из файла
        w_setup.exec(sql_schema);
        w_setup.commit();

        manager = std::make_unique<DBManager::Manager>(TEST_DB_CONNECTION);
    }

    // Выполняется после каждого теста
    void TearDown() override
    {
        // Очищаем таблицы, чтобы не влиять на следующие тесты
        pqxx::work w(*C_check);
        w.exec("TRUNCATE FileOwners, Files, Users, MyUsername RESTART IDENTITY CASCADE;");
        w.commit();
        C_check->disconnect();
        // Удаляем временные файлы
        fs::remove("temp_test_file.txt");
    }

    // Вспомогательная функция для проверки количества записей в таблице
    int count_rows(const std::string &table_name)
    {
        pqxx::work w(*C_check);
        pqxx::result r = w.exec("SELECT COUNT(*) FROM " + table_name);
        return r[0][0].as<int>();
    }
};

// Тест для функции add_user
TEST_F(DBManagerTest, AddUser)
{
    ASSERT_EQ(count_rows("Users"), 0);

    manager->add_user("test_user", true);

    ASSERT_EQ(count_rows("Users"), 1);
    ASSERT_EQ(count_rows("MyUsername"), 1);

    pqxx::work w(*C_check);
    pqxx::result r = w.exec("SELECT username FROM Users WHERE username = 'test_user'");
    ASSERT_FALSE(r.empty());
    ASSERT_EQ(r[0][0].as<std::string>(), "test_user");
}

// Тест для функции add_file
TEST_F(DBManagerTest, AddFile)
{
    // Arrange: создаем пользователя и временный файл
    manager->add_user("file_adder");
    create_temp_file("temp_test_file.txt", "hello world");

    ASSERT_EQ(count_rows("Files"), 0);
    ASSERT_EQ(count_rows("FileOwners"), 0);

    // Act: добавляем файл
    manager->add_file("./temp_test_file.txt", "test.txt", "/docs", "file_adder");

    // Assert: проверяем, что файл и владелец добавлены в БД
    ASSERT_EQ(count_rows("Files"), 1);
    ASSERT_EQ(count_rows("FileOwners"), 1);

    pqxx::work w(*C_check);
    pqxx::result r_file = w.exec("SELECT file_name, DecRep_path, file_size FROM Files");
    ASSERT_EQ(r_file[0]["file_name"].as<std::string>(), "test.txt");
    ASSERT_EQ(r_file[0]["DecRep_path"].as<std::string>(), "/docs");
    ASSERT_EQ(r_file[0]["file_size"].as<long>(), 11); // "hello world" = 11 байт
}

// Тест для функции rename_DecRep_file
TEST_F(DBManagerTest, RenameDecRepFile)
{
    // Arrange
    manager->add_user("renamer");
    create_temp_file("temp_test_file.txt", "content");
    manager->add_file("./temp_test_file.txt", "old_name.txt", "/project", "renamer");

    // Act
    manager->rename_DecRep_file("/project", "old_name.txt", "new_name.txt");

    // Assert
    pqxx::work w(*C_check);
    pqxx::result r = w.exec_params(
        "SELECT 1 FROM Files WHERE DecRep_path = $1 AND file_name = $2",
        "/project", "new_name.txt"
    );
    ASSERT_FALSE(r.empty()); // Убеждаемся, что запись с новым именем существует

    pqxx::result r_old = w.exec_params(
        "SELECT 1 FROM Files WHERE DecRep_path = $1 AND file_name = $2",
        "/project", "old_name.txt"
    );
    ASSERT_TRUE(r_old.empty()); // Убеждаемся, что запись со старым именем удалена
}

// Тест для функции delete_user
TEST_F(DBManagerTest, DeleteUser)
{
    // Arrange
    manager->add_user("user_to_delete");
    create_temp_file("temp_test_file.txt", "data");
    manager->add_file("./temp_test_file.txt", "file.txt", "/home", "user_to_delete");

    ASSERT_EQ(count_rows("Users"), 1);
    ASSERT_EQ(count_rows("Files"), 1);

    // Act
    manager->delete_user("user_to_delete");

    // Assert: пользователь и его файлы (если он был единственным владельцем) удалены
    ASSERT_EQ(count_rows("Users"), 0);
    ASSERT_EQ(count_rows("Files"), 0);
    ASSERT_EQ(count_rows("FileOwners"), 0);
}

// Тест для функции delete_local_file, когда пользователь - единственный владелец
TEST_F(DBManagerTest, DeleteLocalFileAsSoleOwner)
{
    // Arrange
    manager->add_user("owner");
    create_temp_file("temp_test_file.txt", "data");
    manager->add_file("./temp_test_file.txt", "file.txt", "/repo", "owner");

    ASSERT_EQ(count_rows("Files"), 1);
    ASSERT_EQ(count_rows("FileOwners"), 1);

    // Act: локальный файл "удаляется"
    std::string deleted_path = manager->delete_local_file("./temp_test_file.txt", "owner");

    // Assert: так как владелец был один, файл полностью удаляется из БД
    ASSERT_EQ(count_rows("Files"), 0);
    ASSERT_EQ(count_rows("FileOwners"), 0);
    ASSERT_EQ(deleted_path, "/repo/file.txt");
}

// --------------------------------------------------------------------------------

// Тест для функции add_folder
TEST_F(DBManagerTest, AddFolder)
{
    // Arrange
    fs::create_directory("temp_test_folder");
    create_temp_file("temp_test_folder/file1.txt", "content1");
    create_temp_file("temp_test_folder/file2.txt", "content2");

    manager->add_user("folder_adder");
    ASSERT_EQ(count_rows("Files"), 0);
    ASSERT_EQ(count_rows("FileOwners"), 0);

    // Act
    manager->add_folder("./temp_test_folder", "/docs", "folder_adder");

    ASSERT_EQ(count_rows("Files"), 2);
    ASSERT_EQ(count_rows("FileOwners"), 2);

    // Проверяем содержимое БД
    pqxx::work w(*C_check);
    pqxx::result r = w.exec("SELECT file_name FROM Files");
    std::set<std::string> files;
    for (const auto &row : r) {
        files.insert(row[0].as<std::string>());
    }
    ASSERT_TRUE(files.find("file1.txt") != files.end());
    ASSERT_TRUE(files.find("file2.txt") != files.end());

    fs::remove_all("temp_test_folder"); // Удаляем временную папку
}

// Тест для функции rename_DecRep_folder
TEST_F(DBManagerTest, RenameDecRepFolder)
{
    // Arrange
    manager->add_user("user");
    create_temp_file("temp_test_file.txt", "content");
    manager->add_file("./temp_test_file.txt", "file.txt", "/old_folder", "user");

    // Act
    manager->rename_DecRep_folder("/old_folder", "/new_folder");

    pqxx::work w(*C_check);
    pqxx::result r = w.exec("SELECT DecRep_path FROM Files");
    ASSERT_EQ(r[0][0].as<std::string>(), "/new_folder");
}

// Тест для функции change_DecRep_path
TEST_F(DBManagerTest, ChangeDecRepPath)
{
    // Arrange
    manager->add_user("user");
    create_temp_file("temp_test_file.txt", "content");
    manager->add_file("./temp_test_file.txt", "file.txt", "/old", "user");

    // Act
    manager->change_DecRep_path("file.txt", "/old", "/new");

    pqxx::work w(*C_check);
    pqxx::result r = w.exec("SELECT DecRep_path FROM Files WHERE file_name = 'file.txt'");
    ASSERT_EQ(r[0][0].as<std::string>(), "/new");
}

// Тест для функции untrack_file
TEST_F(DBManagerTest, UntrackFile)
{
    // Arrange
    manager->add_user("user");
    create_temp_file("temp_test_file.txt", "content");
    manager->add_file("./temp_test_file.txt", "file.txt", "/docs", "user");

    ASSERT_EQ(count_rows("Files"), 1);
    ASSERT_EQ(count_rows("FileOwners"), 1);

    // Act
    manager->untrack_file("/docs/file.txt");

    ASSERT_EQ(count_rows("Files"), 0);
    ASSERT_EQ(count_rows("FileOwners"), 0);
}

// Тест для функции untrack_folder
TEST_F(DBManagerTest, UntrackFolder)
{
    // Arrange
    manager->add_user("user");
    create_temp_file("temp_test_file1.txt", "content1");
    create_temp_file("temp_test_file2.txt", "content2");
    manager->add_file("./temp_test_file1.txt", "file1.txt", "/folder", "user");
    manager->add_file("./temp_test_file2.txt", "file2.txt", "/folder", "user");

    ASSERT_EQ(count_rows("Files"), 2);
    ASSERT_EQ(count_rows("FileOwners"), 2);

    // Act
    manager->untrack_folder("/folder");

    ASSERT_EQ(count_rows("Files"), 0);
    ASSERT_EQ(count_rows("FileOwners"), 0);
}

// Тест для функции download_file
TEST_F(DBManagerTest, DownloadFile)
{
    // Arrange
    manager->add_user("user1");
    create_temp_file("temp_test_file.txt", "content");
    manager->add_file("./temp_test_file.txt", "file.txt", "/shared", "user1");
    manager->add_user("user2");

    // Act
    manager->download_file("user2", "/shared/file.txt", "./downloaded.txt");

    // Проверяем добавление записи о новом владельце
    ASSERT_EQ(count_rows("FileOwners"), 2);

    pqxx::work w(*C_check);
    pqxx::result r = w.exec(
        "SELECT local_path FROM FileOwners "
        "WHERE owner_id = (SELECT id FROM Users WHERE username = 'user2')"
    );
    ASSERT_EQ(r[0][0].as<std::string>(), "./downloaded.txt");
}

// Тест для функции update_local_file_path
TEST_F(DBManagerTest, UpdateLocalFilePath)
{
    // Arrange
    manager->add_user("user");
    create_temp_file("temp_test_file.txt", "content");
    manager->add_file("./temp_test_file.txt", "file.txt", "/docs", "user");

    // Act
    manager->update_local_file_path(
        "./temp_test_file.txt",
        "./new_location.txt",
        "user"
    );

    pqxx::work w(*C_check);
    pqxx::result r = w.exec(
        "SELECT local_path FROM FileOwners "
        "WHERE owner_id = (SELECT id FROM Users WHERE username = 'user')"
    );
    ASSERT_EQ(r[0][0].as<std::string>(), "./new_location.txt");
}

// Тест для функции update_file (изменение содержимого)
TEST_F(DBManagerTest, UpdateFile)
{
    // Arrange
    manager->add_user("user1");
    manager->add_user("user2");
    create_temp_file("temp_test_file.txt", "initial content");

    // user1 добавляет файл
    manager->add_file("./temp_test_file.txt", "file.txt", "/docs", "user1");

    // user2 скачивает файл
    manager->download_file("user2", "/docs/file.txt", "./user2_copy.txt");

    // Изменяем содержимое файла
    create_temp_file("temp_test_file.txt", "updated and longer content");

    // user1 обновляет файл
    manager->update_file("./temp_test_file.txt", "user1");

    // Проверяем обновление данных
    pqxx::work w(*C_check);

    // Проверяем размер файла
    pqxx::result r_size = w.exec("SELECT file_size FROM Files");
    ASSERT_EQ(r_size[0][0].as<long>(), 25); // "updated and longer content"

    // Проверяем, что user2 удален из владельцев
    pqxx::result r_owners = w.exec(
        "SELECT COUNT(*) FROM FileOwners "
        "WHERE file_id = (SELECT id FROM Files) AND "
        "owner_id = (SELECT id FROM Users WHERE username = 'user2')"
    );
    ASSERT_EQ(r_owners[0][0].as<int>(), 0);
}

// Тест для delete_local_file с несколькими владельцами
TEST_F(DBManagerTest, DeleteLocalFileWithMultipleOwners)
{
    manager->add_user("user1");
    manager->add_user("user2");
    create_temp_file("temp_test_file.txt", "shared content");

    manager->add_file("./temp_test_file.txt", "file.txt", "/shared", "user1");
    manager->download_file("user2", "/shared/file.txt", "./user2_copy.txt");

    ASSERT_EQ(count_rows("FileOwners"), 2);

    // user1 удаляет свою копию
    manager->delete_local_file("./temp_test_file.txt", "user1");

    // Файл должен остаться в системе
    ASSERT_EQ(count_rows("Files"), 1);

    // Должен остаться один владелец (user2)
    ASSERT_EQ(count_rows("FileOwners"), 1);

    pqxx::work w(*C_check);
    pqxx::result r = w.exec(
        "SELECT username FROM Users WHERE id = ("
        "SELECT owner_id FROM FileOwners)"
    );
    ASSERT_EQ(r[0][0].as<std::string>(), "user2");
}

// Тест для функции update_local_folder_path
TEST_F(DBManagerTest, UpdateLocalFolderPath)
{
    // Создаем временную структуру
    fs::create_directory("old_folder");
    create_temp_file("old_folder/file1.txt", "content1");
    create_temp_file("old_folder/file2.txt", "content2");

    manager->add_user("user");
    manager->add_folder("./old_folder", "/repo", "user");

    // Подготавливаем данные для обновления
    std::vector<std::string> old_paths = {
        "./old_folder/file1.txt",
        "./old_folder/file2.txt"
    };
    std::vector<std::string> new_paths = {
        "./new_folder/file1.txt",
        "./new_folder/file2.txt"
    };

    manager->update_local_folder_path(old_paths, new_paths, "user");

    // Проверяем обновление путей
    pqxx::work w(*C_check);
    pqxx::result r = w.exec("SELECT local_path FROM FileOwners ORDER BY local_path");

    ASSERT_EQ(r.size(), 2);
    ASSERT_EQ(r[0][0].as<std::string>(), "./new_folder/file1.txt");
    ASSERT_EQ(r[1][0].as<std::string>(), "./new_folder/file2.txt");

    fs::remove_all("old_folder"); // Очистка
}

// Тест для функции get_files_info
TEST_F(DBManagerTest, GetFilesInfo)
{
    manager->add_user("user");
    create_temp_file("temp_test_file.txt", "content");
    manager->add_file("./temp_test_file.txt", "test.txt", "/docs", "user");

    auto files = manager->get_files_info();
    ASSERT_EQ(files.size(), 1);
    ASSERT_EQ(files[0].file_name, "test.txt");
    ASSERT_EQ(files[0].DecRep_path, "/docs");
}

// Тест для функции is_users_empty
TEST_F(DBManagerTest, IsUsersEmpty)
{
    ASSERT_TRUE(manager->is_users_empty());
    manager->add_user("test_user");
    ASSERT_FALSE(manager->is_users_empty());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
