#ifndef SERVER_HPP_
#define SERVER_HPP_

#include "process_events.hpp"
#include <algorithm>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <boost/json/src.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;

namespace Server {
class HTTPServer {
private:
    Events::EventHandler &handler;

public:
    HTTPServer(Events::EventHandler &handler_);

    // Return a response for the given request.
    http::message_generator handle_request(
        http::request<http::string_body> &&req,
        const net::ip::tcp::endpoint &remote_ep
    );

    // Handles an HTTP server connection
    net::awaitable<void> do_session(beast::tcp_stream stream);

    // Accepts incoming connections and launches the sessions
    net::awaitable<void> do_listen(const net::ip::tcp::endpoint &endpoint);
};
}; // namespace Server

#endif // SERVER_HPP_