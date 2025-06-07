#include "client.hpp"
#include "dec_rep.hpp"
#include "process_events.hpp"
#include <format>

#define SERVER_LISTENER_PORT 1498


int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: dec-rep <db_name> <db_password>\n"
                  << "Example:\n"
                  << "    ./dec-rep mydb 123123\n";
        return EXIT_FAILURE;
    }

    const auto connection_str = std::format(
        "host=localhost port=5432 dbname={} user=postgres password={}", argv[2],
        argv[3]
    );

    try {
        DecRep app("0.0.0.0", SERVER_LISTENER_PORT, connection_str);
        app.run();

        // TODO
        // First connection
        if (app.m_db_manager.is_users_empty()) {
            std::string user_name {};
            std::cout << "Hello, enter your user name: ";
            std::cin >> user_name;

            std::string command {};
            std::cout
                << "You don't have a repository yet. Do you want to create "
                   "your own or connect to an existing one?\n"
                << "Type (create) or (connect): ";
            std::cin >> command;

            if (command != "connect" || command != "create") {
                std::cerr << "Incorrect command\n";
                return EXIT_FAILURE;
            }
            if (command == "connect") {
                std::string ip {}, port {};
                std::cout << "Enter host's ip address and port (Ex: 0.0.0.0 1234): ";
                std::cin >> ip >> port;
                net::co_spawn(
                    app.m_ioc,
                    app.m_client.do_session(ip, std::stoi(port), "events/get_db_data/" + user_name, 11),
                    [](std::exception_ptr e) {
                        if (e) {
                            std::rethrow_exception(e);
                        }
                    }
                );
            } else {
                app.m_db_manager.insert_into_Users(user_name, "0.0.0.0", "1234"); // TODO
            }
        }

        std::cout << "App is running...\n";
        std::string line{};
        for (;;) {
            std::cout << "Enter your comands (or type 'help')\n";
            std::getline(std::cin, line);
            std::vector<std::string_view> parts = Events::split_str(line, ' ');
            if (parts.empty()) {
                continue;
            }

            std::string command_name(parts[0]);
            std::vector<std::string_view> command_args;
            if (parts.size() > 1) {
                command_args.assign(parts.begin() + 1, parts.end());
            }

            auto it = app.m_event_handler.func_map.find(command_name);
            if (it != app.m_event_handler.func_map.end()) {
                it->second(command_args);
            } else {
                std::cout << "Unknown command:" << command_name << '\n';
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}