#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace asio = boost::asio;


// для обработки каждого подключения
class HttpSession : public std::enable_shared_from_this<HttpSession> {
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;      //специально для работы с сетевыми буферами
    http::request<http::string_body> req_;

public:
    //передаём принятый сокет в объект класса, который будет обрабатывать это соединение
    explicit HttpSession(tcp::socket socket) : socket_(std::move(socket)){}

    //запуск обработки сессии
    void start() {
      do_read();
    }

private:
    void do_read() {
        //объект не уничтожится, пока выполняется операция
        auto self = shared_from_this();
        //читаем запрос из сокета и передаём в реквест
        http::async_read(socket_, buffer_, req_,
            [self](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    std::cerr << "Error reading HTTP request: " << ec.message() << std::endl;
                    return;
                }
                self->handle_request();
            });
    }

    void handle_request() {

        std::string body = "Recieved: " + req_.body();

        // формирование HTTP-ответа                   //200 OK - запрос успешно обработан и принят
        http::response<http::string_body> res{http::status::ok, req_.version()};
        res.set(http::field::server, "server");
        res.set(http::field::content_type, "text/plain");  //в ответе просто текст
        res.body() = body;
        res.prepare_payload();

        auto self = shared_from_this();
        http::async_write(socket_, res,
            [self](boost::system::error_code ec, std::size_t bytes_transferred) {
                //завершаем соединение
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            });
    }
};

// Класс HTTP сервера для прослушивания входящих подключений
class HttpServer : public std::enable_shared_from_this<HttpServer> {
private:
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    HttpServer(asio::io_context &io_context, tcp::endpoint endpoint)
        : acceptor_(io_context), socket_(io_context)
    {
        boost::system::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Acceptor open error: " << ec.message() << std::endl;
            return;
        }
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "Bind error: " << ec.message() << std::endl;
            return;
        }
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Listen error: " << ec.message() << std::endl;
            return;
        }
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        auto self = shared_from_this();
        acceptor_.async_accept(socket_,
            [self](boost::system::error_code ec) {
                if (ec) {
                std::cerr << "Error in accept: " << ec.message() << std::endl;
            } else {
                try {
                    std::cout << "Accepted connection from " << self->socket_.remote_endpoint() << " --> "  << self->socket_.local_endpoint() << std::endl;
                } catch (const std::exception &e) {
                    std::cerr << "Error retrieving endpoints: " << e.what() << std::endl;
                }
                //начинаем сессию для нового подключения
                std::make_shared<HttpSession>(std::move(self->socket_))->start();
            }
            //продолжаем принимать новые подключения
            self->do_accept();
        });
    }
};

int main() {
    try {
        asio::io_context io_context;
        auto address = asio::ip::make_address("0.0.0.0");
        unsigned short port = 25000;

        std::shared_ptr<HttpServer> server = std::make_shared<HttpServer>(io_context, tcp::endpoint{address, port});
        server->run();
        std::cout << "Server starting. Listening at " << address << ":" << port << std::endl;
        io_context.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
