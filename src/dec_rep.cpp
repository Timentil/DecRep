#include "dec_rep.hpp"

// void DecRep::start_server(const std::string &address, const int port)
// {
//     auto endpoint
//         = net::ip::tcp::endpoint { net::ip::make_address(address),
//                                    static_cast<unsigned short>(port) };

//     // Spawn a listening port
//     net::co_spawn(
//         m_ioc, m_server.do_listen(endpoint),
//         [](std::exception_ptr e) {
//             if (e) {
//                 try {
//                     std::rethrow_exception(e);
//                 } catch (std::exception const &e) {
//                     std::cerr << "Error: " << e.what() << std::endl;
//                 }
//             }
//         }
//     );
// }

void DecRep::construct_dec_rep_fs() {
    auto files = m_db_manager.get_files_info();
    for (auto file : files) {
        m_dec_rep_fs.add_file(file.DecRep_path, file.file_name);
    }
}

DecRep::DecRep(const std::string &address, int port, const std::string &connection_data)
    : m_ioc()
    , m_work_guard(net::make_work_guard(m_ioc))
    , m_db_manager(connection_data)
    , m_dec_rep_fs()
    , m_event_handler(m_db_manager, m_dec_rep_fs)
    // , m_server(m_event_handler)
    , m_client(m_event_handler)
    , m_search_service(m_ioc)
    , m_propagator(m_event_handler, m_client, m_search_service)
    , m_server()
    {
    // start_server(address, port);
    m_search_service.run_service();
    construct_dec_rep_fs();
    // start_file_watcher();
    // soon...
}

DecRep::~DecRep()
{
    stop();
}

void DecRep::run()
{
    m_jthread = std::jthread([this] { m_ioc.run(); });
}

void DecRep::stop()
{
    m_work_guard.reset();
    m_ioc.stop();
}