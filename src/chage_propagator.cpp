#include "change_propagator.hpp"

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
    int max_retryes = 3
)
    : m_event_handler(event_handler)
    , m_client(client)
    , m_search_service(search_service)
    , m_max_retryes(max_retryes)
{
}

net::awaitable<void> ChangePropagator::on_local_change(const std::vector<std::string_view> &parts)
{
    std::string command_name(parts[0]);
    std::vector<std::string_view> command_args;
    if (parts.size() > 1) {
        command_args.assign(parts.begin() + 1, parts.end());
    }

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

    // Получаем ip
    std::set<address> users = m_search_service.get_app_endpoints();

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
