#include "server.hpp"

namespace server {
beast::string_view mime_type(beast::string_view path) {
    using beast::iequals;
    const auto ext = [&path] {
        auto const pos = path.rfind(".");
        if (pos == beast::string_view::npos) {
            return beast::string_view{};
        }
        return path.substr(pos);
    }();
    if (iequals(ext, ".htm")) {
        return "text/html";
    }
    if (iequals(ext, ".html")) {
        return "text/html";
    }
    if (iequals(ext, ".php")) {
        return "text/html";
    }
    if (iequals(ext, ".css")) {
        return "text/css";
    }
    if (iequals(ext, ".txt")) {
        return "text/plain";
    }
    if (iequals(ext, ".js")) {
        return "application/javascript";
    }
    if (iequals(ext, ".json")) {
        return "application/json";
    }
    if (iequals(ext, ".xml")) {
        return "application/xml";
    }
    if (iequals(ext, ".swf")) {
        return "application/x-shockwave-flash";
    }
    if (iequals(ext, ".flv")) {
        return "video/x-flv";
    }
    if (iequals(ext, ".png")) {
        return "image/png";
    }
    if (iequals(ext, ".jpe")) {
        return "image/jpeg";
    }
    if (iequals(ext, ".jpeg")) {
        return "image/jpeg";
    }
    if (iequals(ext, ".jpg")) {
        return "image/jpeg";
    }
    if (iequals(ext, ".gif")) {
        return "image/gif";
    }
    if (iequals(ext, ".bmp")) {
        return "image/bmp";
    }
    if (iequals(ext, ".ico")) {
        return "image/vnd.microsoft.icon";
    }
    if (iequals(ext, ".tiff")) {
        return "image/tiff";
    }
    if (iequals(ext, ".tif")) {
        return "image/tiff";
    }
    if (iequals(ext, ".svg")) {
        return "image/svg+xml";
    }
    if (iequals(ext, ".svgz")) {
        return "image/svg+xml";
    }
    return "application/text";
}

std::string path_cat(beast::string_view base, beast::string_view path) {
    if (base.empty()) {
        return std::string(path);
    }
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if (result.back() == path_separator) {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
    for (auto &c : result) {
        if (c == '/') {
            c = path_separator;
        }
    }
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator) {
        result.resize(result.size() - 1);
    }
    result.append(path.data(), path.size());
#endif
    return result;
}

template <class Body, class Allocator>
http::message_generator handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>> &&req
) {
    // Returns a bad request response
    const auto bad_request = [&req](beast::string_view why) {
        http::response<http::string_body> res{
            http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    const auto not_found = [&req](beast::string_view target) {
        http::response<http::string_body> res{
            http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() =
            "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    const auto server_error = [&req](beast::string_view what) {
        http::response<http::string_body> res{
            http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get) {
        return bad_request("Unknown HTTP-method");
    }

    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos) {
        return bad_request("Illegal request-target");
    }

    // // Build the path to the requested file
    // std::string path = path_cat(doc_root, req.target());
    // if (req.target().back() == '/') {
    //     path.append("index.html");
    // }

    // // Attempt to open the file
    // beast::error_code ec;
    // http::file_body::value_type body;
    // body.open(path.c_str(), beast::file_mode::scan, ec);

    // // Handle the case where the file doesn't exist
    // if (ec == beast::errc::no_such_file_or_directory) {
    //     return not_found(req.target());
    // }

    // Handle an unknown error
    if (ec) {
        return server_error(ec.message());
    }

    // // Cache the size since we need it after the move
    // const auto size = body.size();

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

net::awaitable<void> do_session(
    beast::tcp_stream stream,
    std::shared_ptr<std::string const> doc_root
) {
    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    for (;;) {
        // Set the timeout.
        stream.expires_after(std::chrono::seconds(30));

        // Read a request
        http::request<http::string_body> req;
        co_await http::async_read(stream, buffer, req);

        // Handle the request
        http::message_generator msg = handle_request(*doc_root, std::move(req));

        // Determine if we should close the connection
        bool keep_alive = msg.keep_alive();

        // Send the response
        co_await beast::async_write(stream, std::move(msg));

        if (!keep_alive) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    // Send a TCP shutdown
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_send);

    // At this point the connection is closed gracefully
    // we ignore the error because the client might have
    // dropped the connection already.
}

net::awaitable<void> do_listen(
    net::ip::tcp::endpoint endpoint,
    std::shared_ptr<std::string const> doc_root
) {
    auto executor = co_await net::this_coro::executor;
    auto acceptor = net::ip::tcp::acceptor{executor, endpoint};

    for (;;) {
        net::co_spawn(
            executor,
            do_session(
                beast::tcp_stream{co_await acceptor.async_accept()}, doc_root
            ),
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
}  // namespace server
