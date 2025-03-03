#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

// для обработки каждого подключения
class HttpSession : public std::enable_shared_from_this<HttpSession> {
    tcp::socket socket_;
    boost::beast::flat_buffer
        buffer_;  // специально для работы с сетевыми буферами
    http::request<http::string_body> req_;

public:
    explicit HttpSession(tcp::socket socket) : socket_(std::move(socket)) {
    }

    // запуск обработки сессии
    void start() {
        do_read();
    }

private:
    void do_read() {
        // объект не уничтожится, пока выполняется операция
        auto self = shared_from_this();
        // читаем запрос из сокета и передаём в реквест
        http::async_read(
            socket_, buffer_, req_,
            [self](
                boost::system::error_code ec, std::size_t bytes_transferred
            ) {
                if (ec) {
                    std::cerr << "Error reading HTTP request: " << ec.message()
                              << '\n';
                    return;
                }
                self->handle_request();
            }
        );
    }

    void handle_request() {
        std::string body = "Recieved: " + req_.body();

        // формирование HTTP-ответа
        // 200 OK - запрос успешно обработан и принят
        http::response<http::string_body> res{http::status::ok, req_.version()};
        res.set(http::field::server, "server");
        // в ответе просто текст
        res.set(http::field::content_type, "text/plain");
        res.body() = body;
        // доставляет недостающие заголовки
        res.prepare_payload();
                // начинаем сессию для нового подключения

        // гарантируем, что объект не умрёт до завершения запроса
        auto self = shared_from_this();
        http::async_write(
            socket_, res,
            [self](
                boost::system::error_code ec, std::size_t bytes_transferred
            ) {
                // завершаем соединение
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
            }
        );
    }
};

// Класс HTTP сервера для прослушивания входящих подключений
class HttpServer : public std::enable_shared_from_this<HttpServer> {
private:
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    HttpServer(boost::asio::io_context &io_context, tcp::endpoint endpoint)
        : acceptor_(io_context), socket_(io_context) {
        boost::system::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "Acceptor open error: " << ec.message() << '\n';
            return;
        }
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "Bind error: " << ec.message() << '\n';
            return;
        }
        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Listen error: " << ec.message() << '\n';
            return;
        }
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        auto self = shared_from_this();
        acceptor_.async_accept(socket_, [self](boost::system::error_code ec) {
            if (ec) {
                std::cerr << "Error in accept: " << ec.message() << '\n';
            } else {
                try {
                    std::cout << "Accepted connection from "
                              << self->socket_.remote_endpoint() << " --> "
                              << self->socket_.local_endpoint() << '\n';
                } catch (const std::exception &e) {
                    std::cerr << "Error retrieving endpoints: " << e.what()
                              << '\n';
                }
                // начинаем сессию для нового подключения
                std::make_shared<HttpSession>(std::move(self->socket_))
                    ->start();
            }
            // продолжаем принимать новые подключения
            self->do_accept();
        });
    }
};

int main(int args, char *argv[]) {
    assert(args == 2);
    boost::asio::io_context io_context;

    try {
        std::shared_ptr<HttpServer> server = std::make_shared<HttpServer>(
            io_context, tcp::endpoint(tcp::v4(), std::atoi(argv[1]))
        );
        server->run();
        std::cout << "Server starting. Listening at "
                  << "some ip with port"
                  << ":" << argv[1] << '\n';
        io_context.run();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << '\n';
    }
    return 0;
}
