#ifndef TRANSPORT_SERVICE_HPP
#define TRANSPORT_SERVICE_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <fstream>
#include <utility>

#define SERVER_PORT 6061
#define CLIENT_PORT 6062
#define THREAD_COUNT 1
#define DEC_REP_PATH "./"
#define COMPRESSION_LEVEL Z_BEST_COMPRESSION
#define BUFFER_SIZE 8192


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

namespace transport_service {
    // Сервер предназначен для обработки HTTP-запросов на отправку файлов.
    // На порт, в котором запущен сервер, можно отправлять GET-запросы
    // для получения мета-информации по файлам в репозитории (время создания
    // определенного файла и структура репозитория) и GET-запросы для получения
    // содержимого файла.
    struct Server {
        const int thread_count;
        const int port;
        const std::string dec_rep_path;
        std::thread server_thread;

        [[nodiscard]] http::response<http::string_body> handle_response(
            const http::request<http::string_body> &req
        ) const;

        explicit Server(
            const int m_port = SERVER_PORT,
            const int m_thread_count = THREAD_COUNT,
            std::string m_dec_rep_path = DEC_REP_PATH
        )
            : thread_count(m_thread_count),
              port(m_port),
              dec_rep_path(std::move(m_dec_rep_path)) {
            server_thread = std::thread(&Server::run, this);
            server_thread.detach();
        }

        void do_session(tcp::socket &socket, ssl::context &ctx) const;

        void run() const;
    };

    // Функция для получения файла с сервера по указанному адресу и имени файла.
    void get_file(
        const std::string &server_address,
        const std::string &file_name,
        const std::string &file_path,
        ssl::context &ctx
    );

    void get_large_file(
        const std::string &server_address,
        const std::string &file_name,
        const std::string &file_path,
        ssl::context &ctx
    );

    // Вычисление хеша для валидации файлов.
    std::string sha1_hash_file(const std::string &filename);
    // Функция для сжатия данных с использованием deflate в zlib.
    std::string deflate_compress(const std::string& data);
    // Функция для распаковки данных с использованием deflate в zlib.
    std::string deflate_decompress(const std::string& data);
}  // namespace transport_service

#endif  // TRANSPORT_SERVICE_HPP