#ifndef DEC_REP_FS_HPP_
#define DEC_REP_FS_HPP_

#include "db_manager.hpp"
#include "dec_rep_fs.hpp"

// init database (if it haven't init yet)
// run server (for lisening)
// run file_watcher
// construct DecRepFS from database
// init EventsHandler
class DecRep {
    net::io_context ioc_;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;
    std::thread thread_;
    DBManager::Manager dbManager;
    DecRepFS::DecRepFS decRepFS;

    friend namespace process_events;

public:
    DecRep(const std::string &address, int port, const std::string &doc_root);

    ~DecRep();

    void run();

    void stop();
};

#endif  // DEC_REP_FS_HPP_