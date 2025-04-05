#include "client.hpp"

namespace client {
net::awaitable<http::response<http::dynamic_body>> do_session(
    const std::string &host,
    const std::string &port,
    const std::string &target,
    const std::string &event,
    int version
) {
    auto executor = co_await net::this_coro::executor;
    auto resolver = net::ip::tcp::resolver{executor};
    auto stream = beast::tcp_stream{executor};

    // Look up the domain name
    const auto results = co_await resolver.async_resolve(host, port);

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    co_await stream.async_connect(results);

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set("X-Event-Type", event);

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    co_await http::async_write(stream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    co_await http::async_read(stream, buffer, res);

    // Gracefully close the socket
    beast::error_code ec;
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if (ec && ec != beast::errc::not_connected) {
        throw boost::system::system_error(ec, "shutdown");
    }

    co_return res;
}
}  // namespace client