#include "dec_rep.hpp"

namespace {
using namespace FileWatcher;

void printEvent(const FW_Event &e)
{
    auto printPaths = [](const std::vector<std::string> &old_paths,
                         const std::vector<std::string> &new_paths) {
        size_t n = std::max(old_paths.size(), new_paths.size());
        for (size_t i = 0; i < n; ++i) {
            std::cout << "  ";
            if (i < old_paths.size()) {
                std::cout << "old=" << old_paths[i] << " ";
            }
            if (i < new_paths.size()) {
                std::cout << "new=" << new_paths[i];
            }
            std::cout << "\n";
        }
    };

    switch (e.type) {
    case FW_Event::Type::Added:
        std::cout << "[Added] " << e.new_paths.size() << " file(s):\n";
        printPaths({}, e.new_paths);
        break;
    case FW_Event::Type::Deleted:
        std::cout << "[Deleted] " << e.old_paths.size() << " file(s):\n";
        printPaths(e.old_paths, {});
        break;
    case FW_Event::Type::Modified:
        std::cout << "[Modified] " << e.new_paths.size() << " file(s):\n";
        printPaths({}, e.new_paths);
        break;
    case FW_Event::Type::Moved:
        std::cout << "[Moved] " << e.old_paths.size() << " item(s):\n";
        printPaths(e.old_paths, e.new_paths);
        break;
    }
}
} // namespace

void DecRep::start_server(const std::string &address, const int port)
{
    auto endpoint
        = net::ip::tcp::endpoint { net::ip::make_address(address),
                                   static_cast<unsigned short>(port) };

    // Spawn a listening port
    net::co_spawn(
        m_ioc_http, m_server.do_listen(endpoint),
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

void DecRep::construct_dec_rep_fs()
{
    auto files = m_db_manager.get_files_info();
    for (auto file : files) {
        m_dec_rep_fs.add_file(file.DecRep_path, file.file_name);
    }
}

DecRep::DecRep(const std::string &connection_data)
    : m_ioc_http()
    , m_ioc_propagator()
    , m_ioc_search_service()
    , m_ioc_file_watcher()
    , m_work_guard_http(net::make_work_guard(m_ioc_http))
    , m_work_guard_propagator(net::make_work_guard(m_ioc_propagator))
    , m_work_guard_search_service(net::make_work_guard(m_ioc_search_service))
    , m_work_guard_file_watcher(net::make_work_guard(m_ioc_file_watcher))
    , m_db_manager(connection_data)
    , m_dec_rep_fs()
    , m_event_handler(m_db_manager, m_dec_rep_fs)
    , m_server(m_event_handler)
    , m_client(m_event_handler)
    , m_search_service(m_ioc_search_service)
    , m_propagator(m_event_handler, m_client, m_search_service)
    , m_file_watcher(m_propagator, m_ioc_file_watcher, printEvent)
    , m_server_download()
{
}

DecRep::~DecRep()
{
    stop();
}

void DecRep::run(const std::string &address, int port)
{
    start_server(address, port);
    m_search_service.run_service();
    construct_dec_rep_fs();
    m_file_watcher.run();
    // soon...
    m_jthread_http = std::jthread([this] { m_ioc_http.run(); });
    m_jthread_propagator = std::jthread([this] { m_ioc_propagator.run(); });
    m_jthread_file_watcher = std::jthread([this] { m_ioc_file_watcher.run(); });
}

void DecRep::stop()
{
    m_work_guard_http.reset();
    m_work_guard_propagator.reset();
    m_work_guard_search_service.reset();
    m_work_guard_file_watcher.reset();
    m_ioc_http.stop();
    m_ioc_propagator.stop();
    m_ioc_search_service.stop();
    m_ioc_file_watcher.stop();
}