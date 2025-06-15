#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include "process_events.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

namespace Client {

class HTTPClient {
private:
    Events::EventHandler &handler;

public:
    HTTPClient(Events::EventHandler &handler_);

    void do_session(
        const net::ip::address &address,
        int port,
        const std::string &target
    );
};
} // namespace Client

#endif // CLIENT_HPP_