#include "client.hpp"
#include "dec_rep.hpp"
#include "process_events.hpp"
#include <format>

#define SERVER_LISTENER_PORT 1498

const std::string HELP_MESSAGE = R"(
    Command Reference:

    add_file <local_file_path> <decrep_path> <username>
        Adds a file to be tracked.

    add_folder <local_folder_path> <decrep_path> <username>
        Adds a folder and its contents to be tracked.

    add_user <username>
        Registers a new user.

    update_file <local_file_path> <username>
        Updates the information for a tracked file (e.g., size, modification time).

    update_local_path <old_local_path> <new_local_path> <username>
        Updates the tracked local path of a file.

    untrack_file <full_decrep_path>
        Stops tracking a file entirely from the repository.

    untrack_folder <decrep_path>
        Stops tracking a folder entirely from the repository.

    delete_local_file <local_file_path> <username>
        Stops tracking a local file for a specific user.

    delete_user <username>
        Deletes a user and untracks all their associated files.
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
        DecRep app(CONNECTION_STR);

        // TODO
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
                std::cout << "Sorry, we can't do this yet :(";
                return EXIT_FAILURE;
            } else {
                app.m_db_manager.create_tables();
            }
        }

        app.run("0.0.0.0", SERVER_LISTENER_PORT);
        std::cout << "App is running...\n";
        std::string line {};
        // for(;;) {
        //     std::cin >> line;
        //     transport_service::get_file("192.168.99.12", "test.txt", ".", 0);
        // }
        std::cout << "Enter your comands (or type 'help'):\n";
        for (;;) {
            std::getline(std::cin, line);
            std::vector<std::string_view> parts = Events::split_str(line, ' ');
            if (parts.empty()) {
                continue;
                std::cout << "There's no command\n";
                std::cout << "Enter your comands (or type 'help'):\n";
            }

            std::string command_name(parts[0]);
            if (command_name == "help") {
                std::cout << HELP_MESSAGE << '\n';
                std::cout << "Enter your comands (or type 'help'):\n";
                continue;
            } else if (command_name == "exit") {
                break;
            }

            net::co_spawn(
                app.m_ioc_propagator, app.m_propagator.on_local_change(parts),
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
        return EXIT_SUCCESS;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}