#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/core/noncopyable.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace boost::asio;
using boost::asio::ip::tcp;

class Client : public boost::enable_shared_from_this<Client>, boost::noncopyable {
private:
    static const int message_length = 1024; // Размер буфера для сообщений
    ip::tcp::socket cl_;                     // TCP-сокет для соединения с сервером
    std::vector<char> read_buf_;        // Буфер для записи сообщения

public:
    // Конструктор принимает ссылку на объект io_service (управляет асинхронными операциями) и использует его для инициализации сокета
    Client(boost::asio::io_service& io_context) : cl_(io_context) {
        read_buf_.resize(message_length);
    }


    void start_client() {
        std::string server_ip;
        unsigned short server_port;
        std::cout << "Enter server IP: ";
        std::cin >> server_ip;
        std::cout << "Enter server port: ";
        std::cin >> server_port;

        // Очищаем буфер ввода, чтобы getline работал корректно
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Attempting to connect to " << server_ip << ":" << server_port << std::endl;

        tcp::endpoint endpoint(ip::address::from_string(server_ip), server_port);

        // Асинхронное подключение к серверу по указанному адресу
        cl_.async_connect(
            endpoint,
            //В качестве колбэка указывается функция handle_connect,
            //к которой передаётся указатель на текущий объект (через shared_from_this) и код ошибки подключения
            boost::bind(&Client::handle_connect, shared_from_this(), boost::asio::placeholders::error)
        );
    }

    // Обработчик завершения подключения
    void handle_connect(const boost::system::error_code& err) {
        if (err) {
            std::cout << "Error in connect: " << err << std::endl;
            do_close();
            return;
        }
        // Выводим информацию о соединении: локальный и удалённый endpoints
        try {
            std::cout << "Connected from " << cl_.local_endpoint() << " to " << cl_.remote_endpoint() << std::endl;
        } catch (std::exception &e) {
            std::cout << "Error retrieving endpoints: " << e.what() << std::endl;
        }
        do_write();
    }

    // Функция для отправки данных на сервер
    void do_write() {
        std::cout << "client, input message: " << std::endl;
        // Считываем строку с консоли в буфер write_buf
        std::string input;
        std::getline(std::cin, input);
        std::cout << "# message length: " << input.size() << std::endl;
        // Асинхронная запись данных из буфера в сокет
        async_write(cl_, buffer(input),
                    boost::bind(&Client::handle_write, shared_from_this(), boost::asio::placeholders::error));
    }                      //После завершения записи вызывается handle_write().


    // Обработчик завершения операции записи
    void handle_write(const boost::system::error_code& err) {
        if (err) {
            std::cout << "Write error: " << err << std::endl;
            do_close();
            return;
        }
        // После успешной отправки переходим к чтению ответа от сервера
        do_read_answer();
    }

    // Функция для чтения ответа от сервера
    void do_read_answer() {
        // Асинхронное чтение данных из сокета в буфер write_buf
        cl_.async_read_some(buffer(read_buf_),
                   boost::bind(&Client::handle_read_answer, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    // Обработчик завершения операции чтения
    void handle_read_answer(const boost::system::error_code& err, std::size_t bytes_transferred) {
        if (err) {  // Если произошла ошибка при чтении
            std::cout << "Error in read answer: " << err << std::endl;
            do_close();
            return;
        }

        std::string response(read_buf_.data(), bytes_transferred);
        // Выводим полученное сообщение от сервера
        std::cout << "From server:\n " << std::string(response) << std::endl;
        // После получения ответа переходим к новой отправке
        do_write();
    }


    // Функция для закрытия сокета и завершения работы
    void do_close() {
        std::cout << "Close socket" << std::endl;
        if (cl_.is_open()) {
            cl_.close();
        }
        // Исключение для прерывания работы (например, по таймауту)
        throw std::logic_error("timeout");
    }
};


int main() {
    try {
        boost::asio::io_service io_context; // io_context управляет асинхронными операциями
        boost::shared_ptr<Client> client(new Client(io_context)); // Создаём клиента

        client->start_client(); // Запускаем клиента

        io_context.run(); // Запускаем цикл обработки событий

    } catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return 0;
}