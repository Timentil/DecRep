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


// Server публично наследуется от enable_shared_from_this и приватно(по умолчанию) от noncopyable
// В первом есть метод shared_from_this() -> объект класса Server может создавать объект boost::shared_ptr<Server>, указывающий на себя.
// Это гарантирует, что объект не будет уничтожен, пока выполняется асинхронная операция.
// noncopyable запрещает копирование объектов класса;

class Server : public boost::enable_shared_from_this<Server>, /*private*/ boost::noncopyable {
private:
    static const int message_length = 1024; //константа, определяющая размер буфера. static -> принадлежит всему классу, а не отдельному объекту
    std::vector<char> read_buf_;        //массив для хранения входящих данных от клиента
    tcp::socket ser_;                        //серверный сокет, через который происходит обмен данными
    boost::scoped_ptr<tcp::acceptor> acc_;   //умный указатель (=автоматическое освобождение памяти после удаления объекта Server),
                                            //отвечает за прослушивание входящих сообщений

    //// Сохраняем ссылку на io_context, чтобы использовать его при создании acceptor
    boost::asio::io_context &io_context_;

public:
    // Конструктор принимает ссылку на io_context. : ser(io_context) инициализирует сокет ser объектом io_context
    // Это связывает сокет с циклом обработки событий, через который будут выполняться асинхронные операции.
    Server(boost::asio::io_context &io_context) : ser_(io_context), io_context_(io_context) {
        read_buf_.resize(message_length);
    }

    void start_server(const std::string &listen_ip, unsigned short port) {

        std::cout << "Server starting. Listening at " << listen_ip << ":" << port << std::endl;

        // Создаем конечную точку для прослушивания
        tcp::endpoint endpoint(ip::address::from_string(listen_ip), port);


        // Создаём новый объект приёмника (acceptor) для TCP‑соединений, привязанный к данному endpoint
        acc_.reset(new tcp::acceptor(io_context_, endpoint));


        // Запускаем асинхронное ожидание входящего соединения
        acc_->async_accept(ser_, boost::bind(&Server::handle_accept, shared_from_this(), boost::asio::placeholders::error));
        // handle_accept() — вызывается после! завершения async_accept, когда поступает входящее соединение
    }

    //проверяет успешность принятия соединения
    void handle_accept(const boost::system::error_code &err) {
        //если ошибка -> выводим её и закрываем сокет и приёмник
        if (err) {
            std::cout << "Error in accept: " << err << std::endl;
            do_close();
            return;
        }

        //всё ок -> запускаем асинхронное чтение данных с клиента
        try {
            std::cout << "Accepted connection from " << ser_.remote_endpoint() << " --> " << ser_.local_endpoint() << std::endl;
        } catch (std::exception &e) {
            std::cout << "Error retrieving endpoints: " << e.what() << std::endl;
        }
        do_server_read();
    }

    //инициирует асинхронное чтение
    void do_server_read() {
        ser_.async_read_some(buffer(read_buf_),  //Запускает асинхронное чтение данных из сокета ser в буфер read_buf размером message_length
                   boost::bind(&Server::handle_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }                 // handle_read() — вызывается только после! завершения async_read, когда данные успешно прочитаны или произошла ошибка

    //callback-функция, которая вызывается по завершении операции чтения
    //В ней уже обрабатываются полученные данные или ошибка
    void handle_read(const boost::system::error_code &err, std::size_t bytes_transferred) {
        if (err) {
            std::cout << "Error in read: " << err << std::endl;
            do_close();
            return;
        }

        std::string data(read_buf_.data(), bytes_transferred);
        std::cout << "From client: " << data << std::endl;


        //Ответ клиенту
        std::string response = "Received: " + data;
        async_write(ser_, buffer(response),
                    boost::bind(&Server::handle_write, shared_from_this(), boost::asio::placeholders::error));
    }

    void handle_write(const boost::system::error_code &err) {
        if (err) {
            std::cout << "Error in write: " << err << std::endl;
            do_close();
        }
        else {
            do_server_read();
        }
    }


    //Закрываем сокет
    void do_close() {
        std::cout << "Close socket" << std::endl;
        if (ser_.is_open()) {
            ser_.close();
        }
        //Если объект приёмника существует и открыт, он закрывается
        if (acc_ && acc_->is_open()) {
            acc_->close();
        }
    }
};


int main() {
    try {
        boost::asio::io_context io_context; // через io_context выполняются все асинхронные операции (accept, read, write и т.д.).

        std::string addres = "0.0.0.0";
        unsigned short port = 25000;

        // Создаем объект сервера с передачей ссылки на io_context.
        boost::shared_ptr<Server> server(new Server(io_context));

        //Вызывается метод для запуска сервера: создаётся приёмник и запускается ожидание входящего соединения
        server->start_server(addres, port);

        //Запускается цикл обработки событий, в котором выполняются все асинхронные операции, добавленные в io_context
        io_context.run();

        //Если во время выполнения возникает исключение (например, выброшенное в do_close()), оно перехватывается и выводится сообщение об ошибке.
    } catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return 0;
}


