#include "change_propagator.hpp"
#include "transport_service.hpp"

#ifndef SERVER_LISTENER_PORT
#define SERVER_LISTENER_PORT 1498
#endif

using namespace boost::asio::ip;

namespace ChangePropagator {

std::string join(const std::vector<std::string_view> &parts, char delimiter)
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

void ChangePropagator::on_local_change(const std::vector<std::string_view> &parts)
{
    std::string command_name(parts[0]);
    std::vector<std::string_view> command_args;
    if (parts.size() > 1) {
        command_args.assign(parts.begin() + 1, parts.end());
    }

    std::cout << "CRINGE" << std::endl;

    // Изменяем локально
    auto it = m_event_handler.func_map.find(command_name);
    if (it != m_event_handler.func_map.end()) {
        if (!it->second(command_args)) {
            std::cout << "Invalid args count:" << command_args.size() << '\n';
            return;
        }
    } else {
        std::cout << "Unknown command:" << command_name << '\n';
        return;
    }

    std::cout << "CRINGE" << std::endl;

    // Получаем ip
    // std::set<address> users;
    std::set<address> users = m_search_service.get_app_endpoints();
    // std::cout << users.size() << std::endl;
    std::string target = join(parts, '/'); // TODO возможно не найдётся

    if (command_name == "update_file") {
        for (auto ip : users) {
            std::string file = std::string(parts[1]);
            transport_service::send_file(ip.to_string(), file.substr(1, file.size() - 2), "", 0);
        }
    }


    std::cout << "CRINGE" << std::endl;

    if (command_name == "update_file") {
        for (auto ip : users) {
            std::string file = std::string(parts[1]);
            transport_service::send_file(ip.to_string(), file.substr(1, file.size() - 2), "", 0);
        }
    }

    // Пробрасываем всем request
    for (auto ip : users) {
        // std::thread{
        //     [this](boost::asio::ip::address ip_m, int port, std::string target_m)  {
        //            m_client.do_session(ip_m, port, target_m);         
        //     }, ip, SERVER_LISTENER_PORT, target
        // }.detach();
        m_client.do_session(ip, SERVER_LISTENER_PORT, target);
    }
}

} // namespace ChangePropagator
