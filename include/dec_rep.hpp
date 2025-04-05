#ifndef DEC_REP_HPP_
#define DEC_REP_HPP_

#include "db_manager.hpp"
#include "dec_rep_fs.hpp"
#include "process_events.hpp"
#include "server.hpp"

// run database
// run server (for lisening)
// construct DecRepFS from database
// run file_watcher
class DecRep {
    net::io_context ioc_;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;
    std::thread thread_;
    DBManager::Manager dbManager;
    DecRepFS::DecRepFS decRepFS;

public:
    DecRep(
        const std::string &address,
        int port,
        const std::string &doc_root,
        const std::string &connection_data
    );

    ~DecRep();

    void run();

    void stop();

    net::io_context &get_ioc();

    friend void process_events::connect(DecRep &app, std::istringstream &msg);

    friend void process_events::add_file(
        DecRep &app,
        const std::string &local_file_path,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip
    );

    friend void process_events::add_folder(
        DecRep &app,
        const std::string &local_folder_path,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip
    );

    friend void process_events::add_user(
        DecRep &app,
        const std::string &username,
        const std::string &ip
    );

    friend void process_events::update_file(
        DecRep &app,
        const std::string &local_path,
        const std::string &username,
        const std::string &ip,
        std::string &new_hash,
        std::size_t &new_size
    );

    friend void process_events::update_local_path(
        DecRep &app,
        const std::string &old_local_path,
        const std::string &new_local_path,
        const std::string &username,
        const std::string &ip
    );

    friend void process_events::untrack_file(
        DecRep &app,
        const std::string &full_DecRep_path
    );

    friend void
    process_events::untrack_folder(DecRep &app, const std::string &DecRep_path);

    friend void process_events::delete_local_file(
        DecRep &app,
        const std::string &local_path,
        const std::string &username,
        const std::string &ip
    );

    friend void process_events::delete_user(
        DecRep &app,
        const std::string &username,
        const std::string &ip
    );

    friend bool process_events::is_db_empty(DecRep &app);
};

#endif  // DEC_REP_HPP_