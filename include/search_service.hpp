#ifndef SEARCH_SERVICE_HPP
#define SEARCH_SERVICE_HPP

#include <boost/asio.hpp>
#include <set>
#include <string>

#define LISTENER_PORT 1488

using namespace boost::asio::ip;

namespace search_service {
struct search_service {
    enum run_state { RUN = 0, STOP = 1 };

    run_state state = RUN;
    std::mutex state_lock;
    udp::socket listener_socket;
    udp::socket speaker_socket;
    udp::endpoint listener_endpoint;

    std::thread listener_thread;
    std::thread speaker_thread;

    boost::asio::io_context &io_context;

    const std::string base_message = "DEC REP HERE\n";

    std::set<address> app_endpoints;
    std::mutex new_app_endpoints_lock;
    std::set<address> new_app_endpoints;

    search_service(boost::asio::io_context &io_context, int listener_port);
    explicit search_service(boost::asio::io_context &io_context)
        : search_service(io_context, LISTENER_PORT){};
    void run_listener();
    void run_speaker();
    void run_service();
    void stop_service();

    [[nodiscard]] std::set<address> get_app_endpoints() const {
        return app_endpoints;
    }
};
}  // namespace search_service
#endif  // SEARCH_SERVICE_HPP
