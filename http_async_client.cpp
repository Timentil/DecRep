#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace asio = boost::asio;


int main() {
    try {

        std::string server_ip;
        std::cout << "Enter server IP: ";
        std::cin >> server_ip;

        std::cout << "Enter server port: ";
        std::string port;
        std::cin >> port;

        std::string target = "/";
        int version = 11; // HTTP/1.1

        asio::io_context io_context;


        //подключаемся
        tcp::resolver resolver(io_context);
        auto const results = resolver.resolve(server_ip, port);
        tcp::socket socket(io_context);
        asio::connect(socket, results.begin(), results.end());

        //HTTP GET запрос
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, server_ip);
        req.set(http::field::user_agent, "client");

        //отправляем HTTP запрос
        http::write(socket, req);


        boost::beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        //в res записываем уже обработанный результат
        http::read(socket, buffer, res);

        std::cout << "From server:\n" << res << std::endl;


        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        if(ec && ec != boost::system::errc::not_connected)
            throw boost::system::system_error{ec};

    } catch(std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
