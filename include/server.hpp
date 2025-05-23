#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <boost/json/src.hpp>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "process_events.hpp"

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
    net::awaitable<void> do_listen(net::ip::tcp::endpoint endpoint);
};
};  // namespace Server

#endif  // SERVER_HPP_