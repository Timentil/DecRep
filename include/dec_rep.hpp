#ifndef DEC_REP_HPP_
#define DEC_REP_HPP_

#include "client.hpp"
#include "db_manager.hpp"
#include "dec_rep_fs.hpp"
#include "process_events.hpp"
#include "server.hpp"
#include "search_service.hpp"
#include "change_propagator.hpp"

// - connect to database
// - run server
// - construct DecRepFS from database
// - run file_watcher
// - run search_service
class DecRep {
public:
    net::io_context m_ioc;
    net::executor_work_guard<net::io_context::executor_type> m_work_guard;
    std::jthread m_jthread;

    DBManager::Manager m_db_manager;
    DecRepFS::FS m_dec_rep_fs;
    Events::EventHandler m_event_handler;
    Server::HTTPServer m_server;
    Client::HTTPClient m_client;
    search_service::search_service m_search_service;
    ChangePropagator::ChangePropagator m_propagator;

    DecRep(const std::string &address, int port, const std::string &connection_data);

    void start_server(const std::string &address, const int port);

    void construct_dec_rep_fs();

    void run();

    void stop();

    ~DecRep();
};

#endif // DEC_REP_HPP_