#include "client.hpp"
#include "dec_rep.hpp"
#include "process_events.hpp"
#include <format>

#define SERVER_LISTENER_PORT 1498

const std::string HELP_MESSAGE = R"(
    Справка по командам:
    add_file [локальный_путь_к_файлу] [путь_в_DecRep] [имя_пользователя] - Добавляет файл под отслеживание.
    add_folder [локальный_путь_к_папке] [путь_в_DecRep] [имя_пользователя] - Добавляет папку под отслеживание.
    add_user [имя_пользователя] - Регистрирует нового пользователя.
    update_file [локальный_путь_к_файлу] [имя_пользователя] [новый_размер] - Обновляет информацию об отслеживаемом файле.
    update_local_path [старый_локальный_путь] [новый_локальный_путь] [имя_пользователя] - Обновляет локальный путь.
    untrack_file [полный_путь_в_DecRep] - Прекращает отслеживание файла.
    untrack_folder [путь_в_DecRep] - Прекращает отслеживание папки.
    delete_local_file [локальный_путь_к_файлу] [имя_пользователя] - Удаляет локальный файл.
    delete_user [имя_пользователя] - Удаляет пользователя.
    )";

int main(int argc, char *argv[])
{
    if (argc != 4) {
        std::cerr << "Usage: dec-rep <db_name> <db_user> <user_password>\n"
                  << "Example:\n"
                  << "    ./dec-rep mydb 123123\n";
        return EXIT_FAILURE;
    }

    const auto CONNECTION_STR = std::format(
        "host=localhost port=5432 dbname={} user={} password={}", argv[1], argv[2],
        argv[3]
    );

    try {
        DecRep app("0.0.0.0", SERVER_LISTENER_PORT, CONNECTION_STR);
        app.run();

        // TODO
        // First connection
        // if (app.m_db_manager.is_users_empty()) {
        //     std::string user_name {};
        //     std::cout << "Hello, enter your user name: ";
        //     std::cin >> user_name;

        //     std::string command {};
        //     std::cout
        //         << "You don't have a repository yet. Do you want to create "
        //            "your own or connect to an existing one?\n"
        //         << "Type (create) or (connect): ";
        //     std::cin >> command;

        //     if (command != "connect" || command != "create") {
        //         std::cerr << "Incorrect command\n";
        //         return EXIT_FAILURE;
        //     }
        //     if (command == "connect") {
        //         std::string ip {}, port {};
        //         std::cout << "Enter host's ip address and port (Ex: 0.0.0.0 1234): ";
        //         std::cin >> ip >> port;
        //         net::co_spawn(
        //             app.m_ioc,
        //             app.m_client.do_session(ip, std::stoi(port), "events/get_db_data/" + user_name),
        //             [](std::exception_ptr e) {
        //                 if (e) {
        //                     std::rethrow_exception(e);
        //                 }
        //             }
        //         );
        //     } else {
        //         app.m_db_manager.insert_into_Users(user_name, "0.0.0.0", "1234"); // TODO
        //     }
        // }

        std::cout << "App is running...\n";
        std::string line {};
        // for(;;) {
        //     std::cin >> line;
        //     transport_service::get_file("192.168.99.12", "test.txt", ".", 0);
        // }
        for (;;) {
            std::cout << "Enter your comands (or type 'help')\n";
            std::getline(std::cin, line);
            std::vector<std::string_view> parts = Events::split_str(line, ' ');
            if (parts.empty()) {
                continue;
            }

            std::string command_name(parts[0]);
            if (command_name == "help") {
                std::cout << HELP_MESSAGE << '\n';
                continue;
            }
            app.m_propagator.on_local_change(parts);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}