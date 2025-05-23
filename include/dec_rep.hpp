#ifndef DEC_REP_HPP_
#define DEC_REP_HPP_

#include "db_manager.hpp"
#include "dec_rep_fs.hpp"
#include "process_events.hpp"
#include "server.hpp"
#include "client.hpp"

// run database
// run server (for lisening)
// construct DecRepFS from database
// run file_watcher
class DecRep {
public:
    net::io_context ioc_;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;
    std::thread thread_;
    DBManager::Manager db_manager;
    DecRepFS::FS dec_rep_fs;
    Events::EventHandler event_handler;
    Server::HTTPServer server;
    Client::HTTPClient client;

    DecRep(
        const std::string &address,
        int port,
        const std::string &connection_data
    );

    ~DecRep();

    void run();

    void stop();
};

#endif  // DEC_REP_HPP_