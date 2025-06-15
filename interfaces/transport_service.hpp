#ifndef TRANSPORT_SERVICE_HPP
#define TRANSPORT_SERVICE_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <utility>
#include <zlib.h>

#define SERVER_PORT 6061
#define CLIENT_PORT 6062
#define THREAD_COUNT 1
#define DEC_REP_PATH "./"
#define DEFAULT_COMPRESSION_LEVEL Z_BEST_COMPRESSION
#define BUFFER_SIZE 8192
#define DEFAULT_LOG_FILE "transport_service.log"

#define HASH

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

namespace transport_service {
    struct Server_Logger {
    private:
        const std::string log_file_name;
        mutable std::ofstream log_file;
        mutable bool is_open = true;

    public:
        explicit Server_Logger(std::string m_log_file_name = DEFAULT_LOG_FILE) : log_file_name(std::move(m_log_file_name)) {
            log_file.open(log_file_name.c_str());
            if (!log_file) {
                is_open = false;
                std::cerr << "Could not open log file" << std::endl;
            }
        }

        void log(const std::string& message) const {
            if (log_file.is_open()) {
                log_file << message << std::endl;
            } else if (is_open) {
                std::cerr << "Could not open log file" << std::endl;
                is_open = false;
            }
        }
    };

    struct Certificate_Singleton {
    private:
        ssl::context server_context;
        ssl::context client_context;

        Certificate_Singleton() :
            server_context(ssl::context::sslv23_server),
            client_context(ssl::context::sslv23_client)
        {
            server_context.set_options(
                ssl::context::default_workarounds |
                ssl::context::no_sslv2 |
                ssl::context::single_dh_use
            );
            server_context.use_certificate_chain_file("server.crt");
            server_context.use_private_key_file("server.key", ssl::context::pem);
            server_context.use_tmp_dh_file("dhparams.pem");
        }

    public:
        Certificate_Singleton(const Certificate_Singleton&) = delete;
        Certificate_Singleton& operator=(const Certificate_Singleton&) = delete;

        static Certificate_Singleton& get_instance() {
            static Certificate_Singleton instance;
            return instance;
        }

        [[nodiscard]] ssl::context& get_server_context() { return server_context; }
        [[nodiscard]] ssl::context& get_client_context() { return client_context; }
    };


    // Сервер предназначен для обработки HTTP-запросов на отправку файлов.
    // На порт, в котором запущен сервер, можно отправлять GET-запросы
    // для получения мета-информации по файлам в репозитории (время создания
    // определенного файла и структура репозитория) и GET-запросы для получения
    // содержимого файла.
    struct Server {
    private:
        const int thread_count;
        const int port;
        int compression_level = DEFAULT_COMPRESSION_LEVEL;
        const std::string dec_rep_path;
        const Server_Logger logger;
        mutable std::thread server_thread;
        mutable std::atomic<bool> is_running = true;

    public:
        [[nodiscard]] http::response<http::string_body> handle_response(
            const http::request<http::string_body> &req, const std::string &client_address
        ) const;

        explicit Server(
            const int m_port = SERVER_PORT,
            const int m_thread_count = THREAD_COUNT,
            std::string m_dec_rep_path = DEC_REP_PATH,
            const std::string& m_log_file = DEFAULT_LOG_FILE
        )
            : thread_count(m_thread_count),
              port(m_port),
              dec_rep_path(std::move(m_dec_rep_path)),
              logger(m_log_file){
            // Сертификаты для сервера находятся в корневом репозитории.
            try {
                Certificate_Singleton::get_instance();
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                std::cerr << "Fail to load certificate" << std::endl;
                throw std::runtime_error("Fail to load certificate");
            }
            server_thread = std::thread(&Server::inner_run, this);
            server_thread.detach();
        }

        void set_compression_level(int level) {
            // 0 - без сжатия, 1 - максимальная скорость, 9 - максимальное сжатие
            if (Z_BEST_SPEED - 1 > level || level > Z_BEST_COMPRESSION) {
                throw std::invalid_argument("Invalid compression level");
            }
            compression_level = level;
        }

        [[nodiscard]] int get_port() const {
            return port;
        }


        void inner_run() const;
        void run() const {
            is_running = true;
            server_thread = std::thread(&Server::run, this);
            server_thread.detach();
        }
        void stop() const {
            is_running = false;
        }
    private:
        void inner_closer(tcp::acceptor &socket) const;
        void do_session(tcp::socket &socket) const;
    };

    // Функция для получения файла с сервера по указанному адресу и имени файла.
    int get_file_inner(
        const std::string &server_address,
        const std::string &file_name,
        const std::string &file_path,
        unsigned long local_clock
    );


    void get_file(
        const std::string &server_address,
        const std::string &file_name,
        const std::string &file_path,
        unsigned long local_clock
    );

    void get_large_file(
        const std::string &server_address,
        const std::string &file_name,
        const std::string &file_path,
        unsigned long local_clock
    );

    // Функция для отправки файла на сервера по указанному адресу и имени файла.
    void send_file_inner(
        const std::string &server_address,
        const std::string &file_name,
        const std::string &file_path,
        unsigned long local_clock
    );

    void send_file(
        const std::string &server_address,
        const std::string &file_name,
        const std::string &file_path,
        unsigned long local_clock
    );

    void send_large_file(
        const std::string &server_address,
        const std::string &file_name,
        const std::string &file_path,
        unsigned long local_clock
    );

    inline unsigned long long get_local_time(const std::string &file_name) {
        return 0; // TODO Надо походить в БД ручками.
    }

    // Вычисление хеша для валидации файлов.
    std::string sha1_hash_file(const std::string &filename);
    // Функция для сжатия данных с использованием deflate в zlib.
    std::string deflate_compress(const std::string& data, int compression_level);
    // Функция для распаковки данных с использованием deflate в zlib.
    std::string deflate_decompress(const std::string& data);
}  // namespace transport_service

#endif  // TRANSPORT_SERVICE_HPP