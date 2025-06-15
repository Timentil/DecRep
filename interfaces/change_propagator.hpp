#ifndef CHANGE_PROPAGATOR_HPP_
#define CHANGE_PROPAGATOR_HPP_

#include "client.hpp"
#include "process_events.hpp"
#include "search_service.hpp"

namespace ChangePropagator {

class ChangePropagator {
private:
    Events::EventHandler &m_event_handler;
    Client::HTTPClient &m_client;
    search_service::search_service &m_search_service;
    int m_max_retries;

public:
    ChangePropagator(
        Events::EventHandler &event_handler,
        Client::HTTPClient &client,
        search_service::search_service &search_service,
        int max_retries = 3
    );

    net::awaitable<void> on_local_change(const std::vector<std::string_view> &);
};

} // namespace ChangePropagator

#endif // CHANGE_PROPAGATOR_HPP_