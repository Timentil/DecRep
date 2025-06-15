#include "server.hpp"

namespace Server {

HTTPServer::HTTPServer(Events::EventHandler &handler_)
    : handler(handler_) {};

void HTTPServer::do_session(beast::tcp_stream stream)
{
    beast::flat_buffer buffer;

    for (;;) {
        // Wait for incoming requests
        stream.expires_after(std::chrono::seconds(30));
        http::request<http::string_body> req;
        http::read(stream, buffer, req);

        // Handle the request
        http::message_generator msg = handler.handle_request(std::move(req));

        // Determine if we should close the connection
        bool keep_alive = msg.keep_alive();

        // Send the response
        beast::write(stream, std::move(msg));

        if (!keep_alive) {
            break;
        }
    }

    // Send a TCP shutdown
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_send);
}

// void HTTPServer::do_listen(const net::ip::tcp::endpoint &endpoint)
// {

//     auto acceptor = net::ip::tcp::acceptor { executor, endpoint };

//     for (;;) {
//         net::co_spawn(
//             executor,
//             do_session(beast::tcp_stream { co_await acceptor.async_accept() }),
//             [](std::exception_ptr e) {
//                 if (e) {
//                     try {
//                         std::rethrow_exception(e);
//                     } catch (std::exception const &e) {
//                         std::cerr << "Error: " << e.what() << std::endl;
//                     }
//                 }
//             }
//         );
//     }
// }
} // namespace Server
