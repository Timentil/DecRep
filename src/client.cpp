#include "client.hpp"

namespace client {
net::awaitable<void> do_session(
    const std::string &address,
    int port,
    const std::string &target,
    int version
) {
    auto executor = co_await net::this_coro::executor;
    auto stream = beast::tcp_stream{executor};
    net::ip::tcp::endpoint e(net::ip::make_address(address), port);

    stream.expires_after(std::chrono::seconds(30));
    co_await stream.async_connect(e);

    // Send request
    http::request<http::string_body> req{http::verb::get, target, version};
    stream.expires_after(std::chrono::seconds(30));
    co_await http::async_write(stream, req);

    // Read response
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    co_await http::async_read(stream, buffer, res);

    // Handle response
    std::cout << res << std::endl;

    // Close connection
    beast::error_code ec;
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if (ec && ec != beast::errc::not_connected) {
        throw boost::system::system_error(ec, "shutdown");
    }
}
}  // namespace client