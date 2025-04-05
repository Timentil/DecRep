#include <boost/format.hpp>
#include "client.hpp"
#include "dec_rep.hpp"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: dec-rep <lisening_port> <db_port> <db_name> "
                     "<db_password>\n"
                  << "Example:\n"
                  << "    ./dec-rep 1234 4321 mydb 123123\n";
        return EXIT_FAILURE;
    }
    const auto lisening_port = static_cast<unsigned short>(std::atoi(argv[1]));
    const std::string connection_str = 
    (boost::format("host=localhost port=%1% dbname=%2% user=postgres password=%3%") 
        % argv[2] % argv[3] % argv[4]).str();

    try {
        DecRep app("0.0.0.0", lisening_port, "/", connection_str);

        if (process_events::is_db_empty(app)) {
            std::string user_name{};
            std::cout << "Hello, enter your user name: ";
            std::cin >> user_name;
            process_events::add_user(app, user_name, "0.0.0.0");  // TODO

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
                std::string host{}, port{};
                std::cout << "Enter ip address and port (Ex: 0.0.0.0 1234): ";
                std::cin >> host >> port;
                net::co_spawn(
                    app.get_ioc(), client::do_session(host, port, "/", "connect", 11),
                    [](std::exception_ptr e, http::response<http::dynamic_body> res) {
                        if (e) {
                            std::rethrow_exception(e);
                        }
                        if (res.result_int() != 200) {
                            std::cout << "Request fail: " << res.result_int() << '\n';
                        }
                    }
                );
            }
        }

        std::cout << "App is running...\n";
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}