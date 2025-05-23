#include "server.hpp"

namespace Server {

HTTPServer::HTTPServer(Events::EventHandler &handler_) : handler(handler_){};

http::message_generator HTTPServer::handle_request(
    http::request<http::string_body> &&req,
    const net::ip::tcp::endpoint &remote_ep
) {
    // Returns a simple response
    const auto response =
        [&req](http::status status, std::string_view msg = "") {
            http::response<http::string_body> res{status, req.version()};
            res.keep_alive(req.keep_alive());
            if (msg != "") {
                res.body() = std::string(msg);
            }
            res.prepare_payload();
            return res;
        };

    // Make sure we can handle the method
    if (req.method() != http::verb::get) {
        return response(http::status::bad_request, "Unknown HTTP-method");
    }

    std::string_view target = req.target();

    auto get_next_token = [&]() {
        size_t pos = target.find('/');
        std::string token{};
        if (pos != target.npos) {
            token = target.substr(0, pos);
            target.remove_prefix(pos + 1);
        } else {
            throw std::logic_error("[server] get_next_token find npos");
        }
        return token;
    };

    // For now we can handle only "events"
    if (get_next_token() != "events") {
        return response(http::status::bad_request, "Unknown request-target");
    }

    std::string event = get_next_token();
    std::string row_params(target);
    row_params += '/' + remote_ep.address().to_string();
    row_params += '/' + std::to_string(remote_ep.port());
    std::vector<std::string_view> params = Events::split_str(target);

    // Process event and handle target
    if (event == "get_db_data") {
        bool is_event_success = handler.func_map["add_user"](params);
        
        std::string json_str = handler.get_db_data();
        
        if (json_str.empty() || is_event_success) {
            return response(http::status::bad_request, "json_str is empty or event did'n succsses");
        }
        return response(http::status::accepted, json_str);
    } else {
        bool is_event_success = handler.func_map[event](params);

        if (!is_event_success) {
            return response(
                http::status::bad_request,
                "Faled to do event, uncorrect args count"
            );
        } else {
            return response(http::status::accepted, "OK");
        }
    }
}

net::awaitable<void> HTTPServer::do_session(beast::tcp_stream stream) {
    beast::flat_buffer buffer;

    for (;;) {
        // Wait for incoming requests
        stream.expires_after(std::chrono::seconds(30));
        http::request<http::string_body> req;
        co_await http::async_read(stream, buffer, req);

        // Handle the request
        http::message_generator msg =
            handle_request(std::move(req), stream.socket().remote_endpoint());

        // Determine if we should close the connection
        bool keep_alive = msg.keep_alive();

        // Send the response
        co_await beast::async_write(stream, std::move(msg));

        if (!keep_alive) {
            break;
        }
    }

    // Send a TCP shutdown
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_send);
}

net::awaitable<void> HTTPServer::do_listen(net::ip::tcp::endpoint endpoint) {
    auto executor = co_await net::this_coro::executor;
    auto acceptor = net::ip::tcp::acceptor{executor, endpoint};

    for (;;) {
        net::co_spawn(
            executor,
            do_session(beast::tcp_stream{co_await acceptor.async_accept()}),
            [](std::exception_ptr e) {
                if (e) {
                    try {
                        std::rethrow_exception(e);
                    } catch (std::exception const &e) {
                        std::cerr << "Error in session: " << e.what() << "\n";
                    }
                }
            }
        );
    }
}
}  // namespace Server
