#include "change_propagator.hpp"

#ifndef SERVER_LISTENER_PORT
#define SERVER_LISTENER_PORT 1498
#endif

using namespace boost::asio::ip;

namespace ChangePropagator {

static const std::string HELP_MESSAGE = R"(
    Command Reference:

    add_file <local_file_path> <decrep_path> <username>
        Adds a file to be tracked.
        
    delete_local_file <local_file_path> <username>
        Stops tracking a local file for a specific user.

    get_file <other_ip> <file_name> <dir_path> <ver>
        download file in current dir

    add_folder <local_folder_path> <decrep_path> <username>
        Adds a folder and its contents to be tracked.

    change_DecRep_path <file_name> <old_DecRep_path> <new_DecRep_path>
        move file in DecRep file system

    rename_DecRep_folder <old_DecRep_path_name> <new_old_DecRep_path_name>

    rename_DecRep_file <DecRep_path> <old_file_name> <new_file_name>

    untrack_file <full_decrep_path>
        Stops tracking a file entirely from the repository.

    untrack_folder <decrep_path>
        Stops tracking a folder entirely from the repository.

    )";

std::string join(const std::vector<std::string> &parts, char delimiter)
{
    if (parts.empty()) {
        return "";
    }

    std::string result;
    result += parts[0];

    for (size_t i = 1; i < parts.size(); ++i) {
        result += delimiter;
        result += parts[i];
    }
    return result;
}

ChangePropagator::ChangePropagator(
    Events::EventHandler &event_handler,
    Client::HTTPClient &client,
    search_service::search_service &search_service,
    int max_retries
)
    : m_event_handler(event_handler)
    , m_client(client)
    , m_search_service(search_service)
    , m_max_retries(max_retries)
{
}

net::awaitable<void> ChangePropagator::on_local_change(std::string command)
{
    // Обрабатываем строку
    std::vector<std::string> parts = Events::split_str(command, ' ');
    if (parts.empty()) {
        std::cout << "There's no command\n"
                  << "Enter your comands (or type 'help'):\n";
        co_return;
    }

    std::string command_name(parts[0]);
    if (command_name == "help") {
        std::cout << HELP_MESSAGE << '\n'
                  << "Enter your comands (or type 'help'):\n";
        co_return;
    }

    std::vector<std::string> command_args;
    if (parts.size() > 1) {
        command_args.assign(parts.begin() + 1, parts.end());
    }

    // Изменяем локально
    auto it = m_event_handler.func_map.find(command_name);
    if (it != m_event_handler.func_map.end()) {
        if (!it->second(command_args)) {
            std::cout << "Invalid args count: " << command_args.size() << '\n'
                      << "Enter your comands (or type 'help'):\n";
            co_return;
        } else {
            std::cout << "Command \"" << command_name << "\" successfuly ended\n";
            co_return;
        }
    } else {
        std::cout << "Unknown command: " << command_name << '\n'
                  << "Enter your comands (or type 'help'):\n";
        co_return;
    }

    // Получаем ip
    std::set<address> users = m_search_service.get_app_endpoints();
    for (auto el : users) {
        std::cout << el.to_string() << '\n';
    }
    if (users.empty()) {
        std::cout << "SEARCH_SERVICE: EMPTY\n";
    } else {
        std::cout << "SEARCH_SERVICE HAS " << users.size() << " users:\n";
        for (auto el : users) {
            std::cout << el.to_string() << '\n';
        }
    }

    std::string target = join(parts, '/');
    auto executor = co_await net::this_coro::executor;

    // Пробрасываем всем request
    for (auto ip : users) {
        net::co_spawn(
            executor,
            m_client.do_session(ip, SERVER_LISTENER_PORT, target),
            [](std::exception_ptr e) {
                if (e) {
                    std::rethrow_exception(e);
                }
            }
        );
    }
}

} // namespace ChangePropagator
