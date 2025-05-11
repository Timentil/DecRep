#ifndef FILEWATCHER_HPP
#define FILEWATCHER_HPP

#include <boost/asio.hpp>
#include <efsw/efsw.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <filesystem>

struct FW_Event {
    enum class Type { Added, Deleted, Modified, Moved };
    Type        type;
    std::string old_path;   // для Moved
    std::string new_path;
};

class FileWatcher : public efsw::FileWatchListener {
    using EventCallback = std::function<void(const FW_Event&)>;

private:
    static FW_Event::Type to_Event(efsw::Action action);

    boost::asio::io_context&           io_;
    EventCallback                      callback_;
    std::unique_ptr<efsw::FileWatcher> watcher_;

    std::unordered_set<std::string>    watched_files;  // файлы, за которыми следим
    std::unordered_set<std::string>    watched_dirs;   // директории с watch

public:
    FileWatcher(boost::asio::io_context& io, EventCallback cb);
    ~FileWatcher() override;

    // блокирующий
    void run() const;

    void addWatch(const std::string& path);

    // главный метод
    void handleFileAction(
        efsw::WatchID     watch_id,
        const std::string& dir,
        const std::string& filename,
        efsw::Action      action,
        std::string       oldFilename
    ) override;
};

#endif // FILEWATCHER_HPP
