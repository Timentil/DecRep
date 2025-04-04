#include "client.hpp"
#include "db_manager.hpp"
#include "dec_rep_fs.hpp"
#include "process_events.hpp"
#include "server.hpp"

// init database (if it haven't init yet)
// run server (for lisening)
// run file_watcher
// construct DecRepFS from database
// init EventsHandler
class DecRep {
    net::io_context ioc_;  // Общий io_context
    net::executor_work_guard<net::io_context::executor_type> work_guard_;
    std::thread thread_;

public:
    DecRep(const std::string &address, int port, const std::string &doc_root)
        : ioc_(), work_guard_(net::make_work_guard(ioc_)) {
        // Server init
        {
            const auto address_ = net::ip::make_address(address);
            const auto port_ = static_cast<unsigned short>(port);
            const auto doc_root_ = std::make_shared<std::string>(doc_root);

            // Spawn a listening port
            net::co_spawn(
                ioc_,
                server::do_listen(
                    net::ip::tcp::endpoint{address_, port_}, doc_root_
                ),
                [](std::exception_ptr e) {
                    if (e) {
                        try {
                            std::rethrow_exception(e);
                        } catch (std::exception const &e) {
                            std::cerr << "Error: " << e.what() << std::endl;
                        }
                    }
                }
            );
        }
    }

    ~DecRep() {
        stop();
    }

    void run() {
        // Запускаем обработку асинхронных операций в отдельном потоке
        thread_ = std::thread([this] { ioc_.run(); });
    }

    void stop() {
        work_guard_.reset();  // Разрешаем завершение io_context
        ioc_.stop();  // Останавливаем все операции
        if (thread_.joinable()) {
            thread_.join();  // Дожидаемся завершения потока
        }
    }
};

int main(int argc, char * argv[]) {
    if (argc != 2) {
        std::cout << "Argc incorretc\n";
        return EXIT_FAILURE;
    }
    try {
        DecRep app("0.0.0.0", std::atoi(argv[1]), "/");
        // if database haven't tables -> choose between 2 options: create own
        // decrep or connect to other decrep

        app.run();
        std::cout << "Server is running...\n";

        // while (true) {
            // HTTP server -> Event x;
            // reading user's command and call redirect_func
        // }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}