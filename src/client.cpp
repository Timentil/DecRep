#include "client.hpp"

namespace Client {

HTTPClient::HTTPClient(Events::EventHandler &handler_)
    : handler(handler_) {};

net::awaitable<void> HTTPClient::do_session(
    const net::ip::address &address,
    int port,
    const std::string &target
)
{
    auto executor = co_await net::this_coro::executor;
    auto stream = beast::tcp_stream { executor };
    net::ip::tcp::endpoint e(address, port);

    stream.expires_after(std::chrono::seconds(30));
    co_await stream.async_connect(e);

    // Send request
    http::request<http::string_body> req { http::verb::get, target, 11 };
    stream.expires_after(std::chrono::seconds(30));
    co_await http::async_write(stream, req);

    // Read response
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    co_await http::async_read(stream, buffer, res);

    // Handle response
    if (res.result() == http::status::accepted) {
        handler.handle_response(std::move(res));
    } else {
        throw std::logic_error("[client]: response back without acception");
    }

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
} // namespace Client