#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>

#include "process_events.hpp"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

namespace Client {

class HTTPClient {
private:
    Events::EventHandler &handler;

public:
    HTTPClient(Events::EventHandler &handler_);

    net::awaitable<void> do_session(
        const std::string &address,
        int port,
        const std::string &target,
        int version
    );
};
}  // namespace Client

#endif  // CLIENT_HPP_