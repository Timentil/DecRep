#include "server.hpp"

namespace Server {

template <class Body, class Allocator>
http::message_generator HTTPServer::handle_request(
    http::request<Body, http::basic_fields<Allocator>> &&req,
    const net::ip::tcp::endpoint &remote_ep
) {
    // Returns a bad request response
    const auto bad_request = [&req](beast::string_view why) {
        http::response<http::string_body> res{
            http::status::bad_request, req.version()};
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get) {
        return bad_request("Unknown HTTP-method");
    }

    std::istringstream iss(req.target());
    std::stringstream msg{};
    auto get_next_token = [&]() {
        std::string token{};
        getline(oss, token, '/');
        return token;
    };

    // Handle target
    if (get_next_token == "events") {
        msg << get_next_token;
        msg << remote_ep.address().to_string();
        msg << remote_ep.port();
        handle_message(msg);
    } else {
        bad_request("Unknown request-target");
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct, std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
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
