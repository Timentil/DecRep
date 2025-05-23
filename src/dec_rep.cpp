#include "dec_rep.hpp"

DecRep::DecRep(
    const std::string &address,
    int port,
    const std::string &connection_data
)
    : ioc_(),
      work_guard_(net::make_work_guard(ioc_)),
      db_manager(connection_data),
      event_handler(db_manager, dec_rep_fs),
      server(event_handler) {
    const auto address_ = net::ip::make_address(address);
    const auto port_ = static_cast<unsigned short>(port);

    // Spawn a listening port
    net::co_spawn(
        ioc_, server.do_listen(net::ip::tcp::endpoint{address_, port_}),
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

    // Construct dec_rep_fs from DB
    // soon...
}

DecRep::~DecRep() {
    stop();
}

void DecRep::run() {
    thread_ = std::thread([this] { ioc_.run(); });
}

void DecRep::stop() {
    work_guard_.reset();
    ioc_.stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}