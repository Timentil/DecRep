#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

int main(int args, char *argv[]) {
    assert(args == 3);
    boost::asio::io_context io_context;

    try {
        tcp::socket socket(io_context);
        boost::asio::connect(
            socket, tcp::resolver(io_context).resolve(argv[1], argv[2])
        );

        // HTTP GET запрос
        http::request<http::string_body> req{http::verb::get, "/", 11};
        req.set(http::field::host, argv[1]);
        req.set(http::field::user_agent, "client_test");
        req.body() = R"({"key": "value"})";
        req.prepare_payload(); 

        // отправляем HTTP запрос
        http::write(socket, req);

        boost::beast::flat_buffer buffer;  // сырой ответ
        http::response<http::dynamic_body> res;
        // в res записываем уже обработанный результат
        http::read(socket, buffer, res);

        std::cout << "From server:\n" << res << std::endl;

        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        if (ec && ec != boost::system::errc::not_connected) {
            throw boost::system::system_error{ec};
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
