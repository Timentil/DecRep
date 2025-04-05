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

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

namespace client {
net::awaitable<http::response<http::dynamic_body>> do_session(
    const std::string &host,
    const std::string &port,
    const std::string &target,
    const std::string &event,
    int version
);
}  // namespace client

#endif  // CLIENT_HPP_