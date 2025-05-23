#include "client.hpp"

namespace Client {

HTTPClient::HTTPClient(Events::EventHandler &handler_) : handler(handler_){};

net::awaitable<void> HTTPClient::do_session(
    const std::string &address,
    int port,
    const std::string &target,
    int version
) {
    // Handle event
    std::string_view target_view(target);

    auto get_next_token = [&]() {
        size_t pos = target_view.find('/');
        std::string token{};
        if (pos != target_view.npos) {
            token = target_view.substr(0, pos);
            target_view.remove_prefix(pos + 1);
        } else {
            throw std::logic_error("[client] get_next_token find npos");
        }
        return token;
    };

    std::string event = get_next_token();

    // Process event and handle target
    if (event != "get_db_data") {
        std::string row_params(target_view);
        row_params += '/' + address;
        row_params += '/' + std::to_string(port);
        std::vector<std::string_view> params = Events::split_str(target_view);
        bool is_event_success = handler.func_map[event](params);

        if (!is_event_success) {
            throw std::runtime_error("[client] event didn't success");
        }
    }

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
    http::response<http::string_body> res;
    co_await http::async_read(stream, buffer, res);

    // Handle response
    if (res.result() != http::status::accepted) {
        std::cout << res.body() << '\n';
    }

    // Deserealization json_str
    if (event == "get_db_data") {
        handler.import_data(res.body());
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
}  // namespace Client