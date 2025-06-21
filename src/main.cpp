#include "client.hpp"
#include "dec_rep.hpp"
#include "process_events.hpp"
#include <format>

#define SERVER_LISTENER_PORT 1498

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
        DecRep app(CONNECTION_STR);

        // First connection
        if (!app.m_db_manager.tables_exists()) {
            std::string user_name {};
            std::cout << "Hello, enter your user name: ";
            std::cin >> user_name;

            std::string command {};
            std::cout
                << "You don't have a repository yet. Do you want to create "
                   "your own or connect to an existing one?\n"
                << "Type (create) or (connect): ";
            std::cin >> command;

            if (command != "connect" && command != "create") {
                std::cerr << "Incorrect command\n";
                return EXIT_FAILURE;
            }
            if (command == "connect") {
                // std::string ip {}, port {};
                // std::cout << "Enter host's ip address and port (Ex: 0.0.0.0 1234): ";
                // std::cin >> ip >> port;
                // net::co_spawn(
                //     app.m_ioc,
                //     app.m_client.do_session(ip, std::stoi(port), "events/get_db_data/" + user_name),
                //     [](std::exception_ptr e) {
                //         if (e) {
                //             std::rethrow_exception(e);
                //         }
                //     }
                // );
                std::cout << "Sorry, we can't do this yet :("; // TODO
                return EXIT_FAILURE;
            } else {
                app.m_db_manager.create_tables();
                app.m_db_manager.add_user(user_name, true);
            }
        }

        app.run("0.0.0.0", SERVER_LISTENER_PORT);
        std::cout << "App is running...\n"
                  << "Enter your comands (or type 'help'):\n";
        for (;;) {
            std::string line {};
            std::getline(std::cin, line);
            if (line == "exit") {
                break;
            } else if (line == "print") {
                app.m_dec_rep_fs.print_DecRepFS();
                continue;
            }
            net::co_spawn(
                app.m_ioc_propagator, app.m_propagator.on_local_change(std::move(line)),
                [](std::exception_ptr e) {
                    if (e) {
                        try {
                            std::rethrow_exception(e);
                        } catch (std::exception const &e) {
                            std::cerr << "Error: " << e.what() << std::endl;
                        }
                    }
                }
            );
        }
        app.stop();
        std::cout << "App successfuly stoped\n";
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}