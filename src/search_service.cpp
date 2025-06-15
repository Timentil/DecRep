#include "search_service.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>

using namespace boost::asio::ip;

namespace search_service {
search_service::search_service(
    boost::asio::io_context &io_context,
    const int listener_port
)
    : listener_socket(
          udp::socket(io_context, udp::endpoint(udp::v4(), listener_port))
      ),
      speaker_socket(udp::socket(io_context)),
      io_context(io_context) {
    listener_endpoint = udp::endpoint(address_v4::broadcast(), listener_port);
    speaker_socket.open(udp::v4());
    speaker_socket.set_option(udp::socket::broadcast(true));
}

void search_service::run_listener() {
    run_state state_copy = RUN;
    try {
        while (state_copy == RUN) {
            char data[1024];

            udp::endpoint sender_endpoint;
            const size_t length = listener_socket.receive_from(
                boost::asio::buffer(data), sender_endpoint
            );
            if (std::string(data, length) == base_message) {
                if (!app_endpoints.contains(sender_endpoint.address())) {
                    while (!new_app_endpoints_lock.try_lock()) {
                        app_endpoints.emplace(sender_endpoint.address());
                        new_app_endpoints.emplace(sender_endpoint.address());
                        new_app_endpoints_lock.unlock();
                        break;
                    }
                }
            }
        }
    } catch (boost::system::system_error &e
    ) {  // TODO Попытка закрытия сокета внутри родительского потока
        return;
    }
    while (!state_lock.try_lock()) {
        state_copy = state;
        state_lock.unlock();
        break;
    }
}

void search_service::run_speaker() {
    run_state state_copy = RUN;
    while (state_copy == RUN) {
        speaker_socket.send_to(
            boost::asio::buffer(base_message), listener_endpoint
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        while (!state_lock.try_lock()) {
            state_copy = state;
            state_lock.unlock();
            break;
        }
    }
}

void search_service::run_service() {
    while (!state_lock.try_lock()) {
        state = RUN;
        state_lock.unlock();
        break;
    }
    listener_thread = std::thread(&search_service::run_listener, this);
    speaker_thread = std::thread(&search_service::run_speaker, this);
    listener_thread.detach();
    speaker_thread.detach();
}

void search_service::stop_service() {
    while (!state_lock.try_lock()) {
        state = STOP;
        state_lock.unlock();
    }
    if (listener_thread.joinable()) {
        listener_thread.join();
    }
    if (speaker_thread.joinable()) {
        speaker_thread.join();
    }

    listener_socket.close();

    while (!new_app_endpoints_lock.try_lock()) {
        new_app_endpoints.clear();
        new_app_endpoints_lock.unlock();
        break;
    }
}
}  // namespace search_service
