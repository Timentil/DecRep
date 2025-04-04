#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <algorithm>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

namespace server {
// Return a reasonable mime type based on the extension of a file.
beast::string_view mime_type(beast::string_view path);

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(beast::string_view base, beast::string_view path);

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
http::message_generator handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>> &&req
);

// Handles an HTTP server connection
net::awaitable<void> do_session(
    beast::tcp_stream stream,
    std::shared_ptr<std::string const> doc_root
);

// Accepts incoming connections and launches the sessions
net::awaitable<void> do_listen(
    net::ip::tcp::endpoint endpoint,
    std::shared_ptr<std::string const> doc_root
);
}  // namespace server

#endif // SERVER_HPP_