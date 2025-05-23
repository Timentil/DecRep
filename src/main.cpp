#include <format>
#include "client.hpp"
#include "dec_rep.hpp"

namespace net = boost::asio;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: dec-rep <lisening_port> <db_name> <db_password>\n"
                  << "Example:\n"
                  << "    ./dec-rep 7070 mydb 123123\n";
        return EXIT_FAILURE;
    }

    const auto lisening_port = std::atoi(argv[1]);
    const auto connection_str = std::format(
        "host=localhost port=5432 dbname={} user=postgres password={}", argv[2],
        argv[3]
    );

    try {
        DecRep app("0.0.0.0", lisening_port, connection_str);

        // First connection
        if (app.db_manager.is_users_empty()) {
            std::string user_name{};
            std::cout << "Hello, enter your user name: ";
            std::cin >> user_name;

            std::string command{};
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
                std::string ip{}, port{};
                std::cout << "Enter host's ip address and port (Ex: 0.0.0.0 1234): ";
                std::cin >> ip >> port;
                net::co_spawn(
                    app.ioc_,
                    app.client.do_session(ip, std::stoi(port), "events/get_db_data/" + user_name, 11),
                    [](std::exception_ptr e) {
                        if (e) {
                            std::rethrow_exception(e);
                        }
                    }
                );
            } else {
                app.db_manager.insert_into_Users(user_name, "0.0.0.0", "1234"); // TODO
            }
        }

        std::cout << "App is running...\n"
                  << "Enter your comands here\n";
        // TODO
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}